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
    loader->mutex = xSemaphoreCreateRecursiveMutex();
    loader->app_stack_index = -1;
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

LoaderStatus loader_start_app(const char* id, bool blocking) {
    LoaderMessageLoaderStatusResult result = {
        .value = LoaderStatusOk
    };

    LoaderMessage message = {
        .type = LoaderMessageTypeAppStart,
        .start.id = id,
        .status_value = &result,
        .api_lock = blocking ? api_lock_alloc_locked() : NULL
    };

    furi_message_queue_put(loader->queue, &message, FuriWaitForever);

    if (blocking) {
        api_lock_wait_unlock_and_free(message.api_lock);
    }

    return result.value;
}

void loader_stop_app() {
    LoaderMessage message = {.type = LoaderMessageTypeAppStop};
    furi_message_queue_put(loader->queue, &message, FuriWaitForever);
}

const AppManifest* _Nullable loader_get_current_app() {
    loader_lock();
    const AppManifest* manifest = (loader->app_stack_index >= 0)
        ? loader->app_stack[loader->app_stack_index].manifest
        : NULL;
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

static const char* app_state_to_string(AppState state) {
    switch (state) {
        case APP_STATE_INITIAL:
            return "initial";
        case APP_STATE_STARTED:
            return "started";
        case APP_STATE_SHOWING:
            return "showing";
        case APP_STATE_HIDING:
            return "hiding";
        case APP_STATE_STOPPED:
            return "stopped";
        default:
            return "?";
    }
}

static void app_set_default_flags(App* app) {
    switch (app->manifest->type) {
        case AppTypeDesktop:
            app->flags.show_statusbar = true;
            app->flags.show_toolbar = false;
            break;
        default:
            app->flags.show_statusbar = true;
            app->flags.show_toolbar = true;
            break;
    }
}

static void app_transition_to_state(App* app, AppState state) {
    if (app->state == APP_STATE_STOPPED) {
        FURI_LOG_I(TAG, "app \"%s\" state: %s", app->manifest->id, app_state_to_string(state));
    } else {
        FURI_LOG_I(
            TAG,
            "app \"%s\" state: %s -> %s",
            app->manifest->id,
            app_state_to_string(app->state),
            app_state_to_string(state)
        );
    }

    switch (state) {
        case APP_STATE_INITIAL:
            app->state = APP_STATE_INITIAL;
            app->context = (Context) {
                .data = NULL
            };
            app_set_default_flags(app);
            // NO-OP
            break;
        case APP_STATE_STARTED:
            if (app->manifest->on_start != NULL) {
                app->manifest->on_start(&app->context);
            }
            app->state = APP_STATE_STARTED;
            break;
        case APP_STATE_SHOWING:
            gui_show_app(
                &app->context,
                app->manifest->on_show,
                app->manifest->on_hide,
                app->flags
            );
            app->state = APP_STATE_SHOWING;
            break;
        case APP_STATE_HIDING:
            gui_hide_app();
            app->state = APP_STATE_HIDING;
            break;
        case APP_STATE_STOPPED:
            if (app->manifest->on_stop) {
                app->manifest->on_stop(&app->context);
            }
            app->state = APP_STATE_STOPPED;
            app->context.data = NULL;
            app->manifest = NULL;
            FURI_LOG_I(TAG, "Free heap: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            break;
    }
}

LoaderStatus loader_do_start_app_with_manifest(
    const AppManifest* _Nonnull manifest
) {
    FURI_LOG_I(TAG, "start with manifest %s", manifest->id);

    loader_lock();

    if (loader->app_stack_index >= (APP_STACK_SIZE - 1)) {
        FURI_LOG_E(TAG, "failed to start app: stack limit of %d reached", APP_STACK_SIZE);
        return LoaderStatusErrorInternal;
    }

    int8_t previous_index = loader->app_stack_index;

    loader->app_stack_index++;
    App* app = &loader->app_stack[loader->app_stack_index];
    app->manifest = manifest;
    app_transition_to_state(app, APP_STATE_INITIAL);
    app_transition_to_state(app, APP_STATE_STARTED);

    // We might have to hide the previous app first
    if (previous_index != -1) {
        App* previous_app = &loader->app_stack[previous_index];
        app_transition_to_state(previous_app, APP_STATE_HIDING);
    }

    app_transition_to_state(app, APP_STATE_SHOWING);

    loader_unlock();

    LoaderEvent event = {.type = LoaderEventTypeApplicationStarted};
    furi_pubsub_publish(loader->pubsub, &event);

    return LoaderStatusOk;
}

static LoaderStatus loader_do_start_by_id(
    const char* id
) {
    FURI_LOG_I(TAG, "Start by id %s", id);

    const AppManifest* manifest = app_manifest_registry_find_by_id(id);
    if (manifest == NULL) {
        return LoaderStatusErrorUnknownApp;
    } else {
        return loader_do_start_app_with_manifest(manifest);
    }
}


static void loader_do_stop_app() {
    loader_lock();

    if (loader->app_stack_index == -1) {
        loader_unlock();
        FURI_LOG_E(TAG, "Stop app: no app running");
        return;
    }

    if (loader->app_stack_index == 0) {
        loader_unlock();
        FURI_LOG_E(TAG, "Stop app: can't stop root app");
        return;
    }

    // Stop current app
    App* app_to_stop = &loader->app_stack[loader->app_stack_index];
    app_transition_to_state(app_to_stop, APP_STATE_HIDING);
    app_transition_to_state(app_to_stop, APP_STATE_STOPPED);

    loader->app_stack_index--;

    // Resume previous app
    App* app_to_resume = &loader->app_stack[loader->app_stack_index];
    app_transition_to_state(app_to_resume, APP_STATE_SHOWING);

    loader_unlock();

    LoaderEvent event = {.type = LoaderEventTypeApplicationStopped};
    furi_pubsub_publish(loader->pubsub, &event);
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
                case LoaderMessageTypeAppStart:
                    message.status_value->value = loader_do_start_by_id(message.start.id);
                    if (message.api_lock) {
                        api_lock_unlock(message.api_lock);
                    }
                    break;
                case LoaderMessageTypeAppStop:
                    loader_do_stop_app();
                    break;
                case LoaderMessageTypeServiceStop:
                    exit_requested = true;
                    break;
            }
        }
    }

    return 0;
}

// region AppManifest

static void loader_start(Context* context) {
    UNUSED(context);
    furi_check(loader == NULL);
    loader = loader_alloc();

    furi_thread_set_priority(loader->thread, FuriThreadPriorityNormal);
    furi_thread_start(loader->thread);
}

static void loader_stop(Context* context) {
    UNUSED(context);
    furi_check(loader != NULL);

    // Send stop signal to thread and wait for thread to finish
    LoaderMessage message = {
        .api_lock = NULL,
        .type = LoaderMessageTypeServiceStop
    };
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