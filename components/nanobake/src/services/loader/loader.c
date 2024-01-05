#include "app_i.h"
#include "app_manifest.h"
#include "app_manifest_registry.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "loader_i.h"
#include "service_manifest.h"
#include "services/gui/gui.h"
#include <sys/cdefs.h>

#define TAG "Loader"

// Forward declarations
static int32_t loader_main(void* p);

static Loader* loader = NULL;

static Loader* loader_alloc() {
    furi_check(loader == NULL);
    loader = malloc(sizeof(Loader));
    loader->pubsub = furi_pubsub_alloc();
    loader->queue = furi_message_queue_alloc(1, sizeof(LoaderMessage));
    loader->thread = furi_thread_alloc_ex(
        "loader",
        4096, // Last known minimum was 2400 for starting Hello World app
        &loader_main,
        NULL
    );
    loader->app_data.args = NULL;
    loader->app_data.app = NULL;
    loader->app_data.view_port = NULL;
    loader->mutex = xSemaphoreCreateRecursiveMutex();
    return loader;
}

static void loader_free() {
    furi_check(loader != NULL);
    furi_thread_free(loader->thread);
    furi_pubsub_free(loader->pubsub);
    furi_message_queue_free(loader->queue);
    furi_mutex_free(loader->mutex);
    free(loader);
}

void loader_lock() {
    furi_assert(loader);
    furi_assert(loader->mutex);
    furi_check(xSemaphoreTakeRecursive(loader->mutex, portMAX_DELAY) == pdPASS);
}

void loader_unlock() {
    furi_assert(loader);
    furi_assert(loader->mutex);
    furi_check(xSemaphoreGiveRecursive(loader->mutex) == pdPASS);
}

LoaderStatus loader_start_app(const char* id, const char* args, FuriString* error_message) {
    LoaderMessage message;
    LoaderMessageLoaderStatusResult result;

    message.type = LoaderMessageTypeStartByName;
    message.start.id = id;
    message.start.args = args;
    message.start.error_message = error_message;
    message.api_lock = api_lock_alloc_locked();
    message.status_value = &result;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
    api_lock_wait_unlock_and_free(message.api_lock);
    return result.value;
}

void loader_start_app_nonblocking(const char* id, const char* args) {
    LoaderMessage message;
    LoaderMessageLoaderStatusResult result;

    message.type = LoaderMessageTypeStartByName;
    message.start.id = id;
    message.start.args = args;
    message.start.error_message = NULL;
    message.api_lock = NULL;
    message.status_value = &result;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

void loader_stop_app() {
    LoaderMessage message;
    message.type = LoaderMessageTypeAppStop;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

const AppManifest* _Nullable loader_get_current_app() {
    loader_lock();
    const App* app = loader->app_data.app;
    const AppManifest* manifest = app ? app->manifest : NULL;
    loader_unlock();

    return manifest;
}

FuriPubSub* loader_get_pubsub() {
    furi_assert(loader);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader->pubsub;
}

static void loader_log_status_error(
    LoaderStatus status,
    FuriString* error_message,
    const char* format,
    va_list args
) {
    if (error_message) {
        furi_string_vprintf(error_message, format, args);
        FURI_LOG_E(TAG, "Status [%d]: %s", status, furi_string_get_cstr(error_message));
    } else {
        FURI_LOG_E(TAG, "Status [%d]", status);
    }
}

static LoaderStatus loader_make_status_error(
    LoaderStatus status,
    FuriString* _Nullable error_message,
    const char* format,
    ...
) {
    va_list args;
    va_start(args, format);
    loader_log_status_error(status, error_message, format, args);
    va_end(args);
    return status;
}

static LoaderStatus loader_make_success_status(FuriString* error_message) {
    if (error_message) {
        furi_string_set(error_message, "App started");
    }

    return LoaderStatusOk;
}

static void loader_start_app_with_manifest(
    const AppManifest* _Nonnull manifest,
    const char* args
) {
    FURI_LOG_I(TAG, "start with manifest %s", manifest->id);

    if (manifest->type != AppTypeUser && manifest->type != AppTypeSystem) {
        furi_crash("App type not supported by loader");
    }

    App* _Nonnull app = furi_app_alloc(manifest);

    loader_lock();

    loader->app_data.app = app;
    loader->app_data.args = (void*)args;

    if (manifest->on_start != NULL) {
        manifest->on_start((void*)args);
    }

    if (manifest->on_show != NULL) {
        ViewPort* view_port = view_port_alloc();
        loader->app_data.view_port = view_port;
        view_port_draw_callback_set(view_port, manifest->on_show, NULL);

        gui_add_view_port(view_port, GuiLayerWindow);
    } else {
        loader->app_data.view_port = NULL;
    }

    loader_unlock();
}

static LoaderStatus loader_do_start_by_id(
    const char* id,
    const char* args,
    FuriString* _Nullable error_message
) {
    FURI_LOG_I(TAG, "Start by id %s", id);

    const AppManifest* manifest = app_manifest_registry_find_by_id(id);
    if (manifest == NULL) {
        return loader_make_status_error(
            LoaderStatusErrorUnknownApp,
            error_message,
            "Application \"%s\" not found",
            id
        );
    }

    loader_start_app_with_manifest(manifest, args);
    return loader_make_success_status(error_message);
}

static void loader_do_stop_app() {
    loader_lock();

    App* app = loader->app_data.app;
    if (app == NULL) {
        FURI_LOG_W(TAG, "Stop app: no app running");
        return;
    }

    FURI_LOG_I(TAG, "Stopping %s", app->manifest->id);

    ViewPort* view_port = loader->app_data.view_port;
    if (view_port) {
        gui_remove_view_port(view_port);
        view_port_free(view_port);
        loader->app_data.view_port = NULL;
    }

    if (app->manifest->on_stop) {
        app->manifest->on_stop();
    }

    if (loader->app_data.args) {
        free(loader->app_data.args);
        loader->app_data.args = NULL;
    }

    furi_app_free(loader->app_data.app);
    loader->app_data.app = NULL;

    loader_unlock();

    FURI_LOG_I(
        TAG,
        "Application stopped. Free heap: %zu",
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL)
    );

    LoaderEvent event;
    event.type = LoaderEventTypeApplicationStopped;
    furi_pubsub_publish(loader->pubsub, &event);
}

bool loader_is_app_running() {
    loader_lock();
    bool is_running = loader->app_data.app != NULL;
    loader_unlock();
    return is_running;
}

static int32_t loader_main(void* p) {
    UNUSED(p);

    LoaderMessage message;
    bool exit_requested = false;
    while (!exit_requested) {
        furi_check(loader != NULL);
        if (furi_message_queue_get(loader->queue, &message, FuriWaitForever) == FuriStatusOk) {
            FURI_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case LoaderMessageTypeStartByName:
                    if (loader_is_app_running()) {
                        loader_do_stop_app();
                    }
                    message.status_value->value = loader_do_start_by_id(
                        message.start.id,
                        message.start.args,
                        message.start.error_message
                    );
                    if (message.api_lock) {
                        api_lock_unlock(message.api_lock);
                    }
                    break;
                case LoaderMessageTypeAppStop:
                    loader_do_stop_app();
                    break;
                case LoaderMessageTypeExit:
                    exit_requested = true;
                    break;
            }
        }
    }

    return 0;
}

// region AppManifest

static void loader_start(void* parameter) {
    UNUSED(parameter);
    furi_check(loader == NULL);
    loader = loader_alloc();

    furi_thread_set_priority(loader->thread, FuriThreadPriorityNormal);
    furi_thread_start(loader->thread);
}

static void loader_stop() {
    furi_check(loader != NULL);
    LoaderMessage message = {
        .api_lock = NULL,
        .type = LoaderMessageTypeExit
    };

    // Send stop signal to thread and wait for thread to finish
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
    furi_thread_join(loader->thread);

    loader_free();
    loader = NULL;
}

const ServiceManifest loader_service = {
    .id = "loader",
    .on_start = &loader_start,
    .on_stop = &loader_stop
};

// endregion