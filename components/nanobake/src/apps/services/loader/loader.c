#include "loader.h"
#include "app_i.h"
#include "app_manifest.h"
#include "app_manifest_registry.h"
#include "loader_i.h"
#include <sys/cdefs.h>
#include "esp_heap_caps.h"
#include "apps/services/gui/gui.h"

#define TAG "Loader"

static int32_t loader_main(void* p);
static LoaderStatus loader_do_start_by_id(
    Loader* loader,
    const char* id,
    const char* args,
    FuriString* error_message
);

LoaderStatus loader_start_app(Loader* loader, const char* id, const char* args, FuriString* error_message) {
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

void loader_start_app_nonblocking(Loader* loader, const char* id, const char* args) {
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

void loader_stop_app(Loader* loader) {
    LoaderMessage message;
    message.type = LoaderMessageTypeAppStop;
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

const AppManifest* _Nullable loader_get_current_app(Loader* loader) {
    App* app = loader->app_data.app;
    if (app != NULL) {
        return app->manifest;
    } else {
        return NULL;
    }
}

FuriPubSub* loader_get_pubsub(Loader* loader) {
    furi_assert(loader);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader->pubsub;
}

// implementation

static Loader* loader_alloc() {
    Loader* loader = malloc(sizeof(Loader));
    loader->pubsub = furi_pubsub_alloc();
    loader->queue = furi_message_queue_alloc(1, sizeof(LoaderMessage));
    loader->thread = furi_thread_alloc_ex(
        "loader",
        AppStackSizeNormal,
        &loader_main,
        NULL
    );
    loader->app_data.args = NULL;
    loader->app_data.app = NULL;
    loader->app_data.view_port = NULL;
    return loader;
}

static void loader_free(Loader* loader) {
    furi_pubsub_free(loader->pubsub);
    furi_message_queue_free(loader->queue);
    furi_thread_free(loader->thread);
    free(loader);
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
    Loader* loader,
    const AppManifest* _Nonnull manifest,
    const char* args
) {
    if (manifest->type != AppTypeUser && manifest->type != AppTypeSystem) {
        furi_crash("app type not supported by loader");
    }

    App* _Nonnull app = furi_app_alloc(manifest);
    loader->app_data.app = app;
    loader->app_data.args = (void*)args;

    if (manifest->on_start != NULL) {
        manifest->on_start((void*)args);
    }

    if (manifest->on_show != NULL) {
        ViewPort* view_port = view_port_alloc();
        loader->app_data.view_port = view_port;
        view_port_draw_callback_set(view_port, manifest->on_show, NULL);

        FURI_RECORD_TRANSACTION(RECORD_GUI, Gui*, gui, {
            gui_add_view_port(gui, view_port, GuiLayerWindow);
        })
    } else {
        loader->app_data.view_port = NULL;
    }
}

// process messages

static LoaderStatus loader_do_start_by_id(
    Loader* loader,
    const char* id,
    const char* args,
    FuriString* _Nullable error_message
) {
    FURI_LOG_I(TAG, "start by id %s", id);

    const AppManifest* manifest = app_manifest_registry_find_by_id(id);
    if (manifest == NULL) {
        return loader_make_status_error(
            LoaderStatusErrorUnknownApp,
            error_message,
            "Application \"%s\" not found",
            id
        );
    }

    loader_start_app_with_manifest(loader, manifest, args);
    return loader_make_success_status(error_message);
}

static void loader_do_stop_app(Loader* loader) {
    App* app = loader->app_data.app;
    if (app == NULL) {
        FURI_LOG_W(TAG, "Stop app: no app running");
        return;
    }

    FURI_LOG_I(TAG, "Stopping %s", app->manifest->id);

    ViewPort* view_port = loader->app_data.view_port;
    if (view_port) {
        FURI_RECORD_TRANSACTION(RECORD_GUI, Gui*, gui, {
            gui_remove_view_port(gui, view_port);
        })
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

    FURI_LOG_I(
        TAG,
        "Application stopped. Free heap: %zu",
        heap_caps_get_free_size(MALLOC_CAP_INTERNAL)
    );

    LoaderEvent event;
    event.type = LoaderEventTypeApplicationStopped;
    furi_pubsub_publish(loader->pubsub, &event);
}

// app

static int32_t loader_main(void* p) {
    UNUSED(p);

    Loader* loader = furi_record_open(RECORD_LOADER);
    furi_record_close(RECORD_LOADER); // TODO: We own it so we can close it safely? Do we need a lock internally?

    LoaderMessage message;
    bool exit_requested = false;
    while (!exit_requested) {
        if (furi_message_queue_get(loader->queue, &message, FuriWaitForever) == FuriStatusOk) {
            FURI_LOG_I(TAG, "processing message of type %d", message.type);
            switch (message.type) {
                case LoaderMessageTypeStartByName:
                    if (loader->app_data.app) {
                        loader_do_stop_app(loader);
                    }
                    message.status_value->value = loader_do_start_by_id(
                        loader,
                        message.start.id,
                        message.start.args,
                        message.start.error_message
                    );
                    if (message.api_lock) {
                        api_lock_unlock(message.api_lock);
                    }
                    break;
                case LoaderMessageTypeAppStop:
                    loader_do_stop_app(loader);
                    break;
                case LoaderMessageTypeExit:
                    exit_requested = true;
                    break;
            }
        }
    }

    return 0;
}

static void loader_start(void* parameter) {
    UNUSED(parameter);
    Loader* loader = loader_alloc();
    furi_record_create(RECORD_LOADER, loader);

    furi_thread_mark_as_service(loader->thread);
    furi_thread_set_priority(loader->thread, FuriThreadPriorityNormal);
    furi_thread_start(loader->thread);
}

static void loader_stop() {
    LoaderMessage message = {
        .api_lock = NULL,
        .type = LoaderMessageTypeExit
    };

    FURI_RECORD_TRANSACTION(RECORD_LOADER, Loader*, loader, {
        furi_message_queue_put(loader->queue, &message, FuriWaitForever);

        furi_thread_join(loader->thread);
        furi_thread_free(loader->thread);
        loader->thread = NULL;

        loader_free(loader);
    })

    furi_record_destroy(RECORD_LOADER);
}

const AppManifest loader_app = {
    .id = "loader",
    .name = "Loader",
    .icon = NULL,
    .type = AppTypeService,
    .on_start = &loader_start,
    .on_stop = &loader_stop,
    .on_show = NULL,
    .stack_size = AppStackSizeNormal
};
