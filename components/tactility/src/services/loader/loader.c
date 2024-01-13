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

static Loader* loader_singleton = NULL;

static Loader* loader_alloc() {
    furi_check(loader_singleton == NULL);
    loader_singleton = malloc(sizeof(Loader));
    loader_singleton->pubsub = furi_pubsub_alloc();
    loader_singleton->queue = furi_message_queue_alloc(1, sizeof(LoaderMessage));
    loader_singleton->thread = furi_thread_alloc_ex(
        "loader",
        4096, // Last known minimum was 2400 for starting Hello World app
        &loader_main,
        NULL
    );
    loader_singleton->mutex = xSemaphoreCreateRecursiveMutex();
    loader_singleton->app_stack_index = -1;
    memset(loader_singleton->app_stack, 0, sizeof(App) * APP_STACK_SIZE);
    return loader_singleton;
}

static void loader_free() {
    furi_check(loader_singleton != NULL);
    furi_thread_free(loader_singleton->thread);
    furi_pubsub_free(loader_singleton->pubsub);
    furi_message_queue_free(loader_singleton->queue);
    furi_mutex_free(loader_singleton->mutex);
    free(loader_singleton);
}

void loader_lock() {
    furi_assert(loader_singleton);
    furi_assert(loader_singleton->mutex);
    furi_check(xSemaphoreTakeRecursive(loader_singleton->mutex, portMAX_DELAY) == pdPASS);
}

void loader_unlock() {
    furi_assert(loader_singleton);
    furi_assert(loader_singleton->mutex);
    furi_check(xSemaphoreGiveRecursive(loader_singleton->mutex) == pdPASS);
}

LoaderStatus loader_start_app(const char* id, bool blocking, Bundle* _Nullable bundle) {
    LoaderMessageLoaderStatusResult result = {
        .value = LoaderStatusOk
    };

    LoaderMessage message = {
        .type = LoaderMessageTypeAppStart,
        .start.id = id,
        .start.bundle = bundle,
        .status_value = &result,
        .api_lock = blocking ? api_lock_alloc_locked() : NULL
    };

    furi_message_queue_put(loader_singleton->queue, &message, FuriWaitForever);

    if (blocking) {
        api_lock_wait_unlock_and_free(message.api_lock);
    }

    return result.value;
}

void loader_stop_app() {
    LoaderMessage message = {.type = LoaderMessageTypeAppStop};
    furi_message_queue_put(loader_singleton->queue, &message, FuriWaitForever);
}

App _Nullable loader_get_current_app() {
    loader_lock();
    App app = (loader_singleton->app_stack_index >= 0)
        ? loader_singleton->app_stack[loader_singleton->app_stack_index]
        : NULL;
    loader_unlock();
    return app;
}

FuriPubSub* loader_get_pubsub() {
    furi_assert(loader_singleton);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader_singleton->pubsub;
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

static void app_transition_to_state(App app, AppState state) {
    const AppManifest* manifest = app_get_manifest(app);
    const AppState old_state = app_get_state(app);

    FURI_LOG_I(
        TAG,
        "app \"%s\" state: %s -> %s",
        manifest->id,
        app_state_to_string(old_state),
        app_state_to_string(state)
    );

    switch (state) {
        case APP_STATE_INITIAL:
            app_set_state(app, APP_STATE_INITIAL);
            break;
        case APP_STATE_STARTED:
            if (manifest->on_start != NULL) {
                manifest->on_start(app);
            }
            app_set_state(app, APP_STATE_STARTED);
            break;
        case APP_STATE_SHOWING:
            gui_show_app(
                app,
                manifest->on_show,
                manifest->on_hide
            );
            app_set_state(app, APP_STATE_SHOWING);
            break;
        case APP_STATE_HIDING:
            gui_hide_app();
            app_set_state(app, APP_STATE_HIDING);
            break;
        case APP_STATE_STOPPED:
            if (manifest->on_stop) {
                manifest->on_stop(app);
            }
            app_set_data(app, NULL);
            app_set_state(app, APP_STATE_STOPPED);
            break;
    }
}

LoaderStatus loader_do_start_app_with_manifest(
    const AppManifest* _Nonnull manifest,
    Bundle* _Nullable bundle
) {
    FURI_LOG_I(TAG, "start with manifest %s", manifest->id);

    loader_lock();

    if (loader_singleton->app_stack_index >= (APP_STACK_SIZE - 1)) {
        FURI_LOG_E(TAG, "failed to start app: stack limit of %d reached", APP_STACK_SIZE);
        return LoaderStatusErrorInternal;
    }

    int8_t previous_index = loader_singleton->app_stack_index;
    loader_singleton->app_stack_index++;

    App app = app_alloc(manifest, bundle);
    furi_assert(loader_singleton->app_stack[loader_singleton->app_stack_index] == NULL);
    loader_singleton->app_stack[loader_singleton->app_stack_index] = app;
    app_transition_to_state(app, APP_STATE_INITIAL);
    app_transition_to_state(app, APP_STATE_STARTED);

    // We might have to hide the previous app first
    if (previous_index != -1) {
        App previous_app = loader_singleton->app_stack[previous_index];
        app_transition_to_state(previous_app, APP_STATE_HIDING);
    }

    app_transition_to_state(app, APP_STATE_SHOWING);

    loader_unlock();

    LoaderEvent event = {.type = LoaderEventTypeApplicationStarted};
    furi_pubsub_publish(loader_singleton->pubsub, &event);

    return LoaderStatusOk;
}

static LoaderStatus loader_do_start_by_id(
    const char* id,
    Bundle* _Nullable bundle
) {
    FURI_LOG_I(TAG, "Start by id %s", id);

    const AppManifest* manifest = app_manifest_registry_find_by_id(id);
    if (manifest == NULL) {
        return LoaderStatusErrorUnknownApp;
    } else {
        return loader_do_start_app_with_manifest(manifest, bundle);
    }
}


static void loader_do_stop_app() {
    loader_lock();

    int8_t current_app_index = loader_singleton->app_stack_index;

    if (current_app_index == -1) {
        loader_unlock();
        FURI_LOG_E(TAG, "Stop app: no app running");
        return;
    }

    if (current_app_index == 0) {
        loader_unlock();
        FURI_LOG_E(TAG, "Stop app: can't stop root app");
        return;
    }

    // Stop current app
    App app_to_stop = loader_singleton->app_stack[current_app_index];
    app_transition_to_state(app_to_stop, APP_STATE_HIDING);
    app_transition_to_state(app_to_stop, APP_STATE_STOPPED);

    app_free(app_to_stop);
    loader_singleton->app_stack[current_app_index] = NULL;
    loader_singleton->app_stack_index--;

    FURI_LOG_I(TAG, "Free heap: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

    // Resume previous app
    furi_assert(loader_singleton->app_stack[loader_singleton->app_stack_index] != NULL);
    App app_to_resume = loader_singleton->app_stack[loader_singleton->app_stack_index];
    app_transition_to_state(app_to_resume, APP_STATE_SHOWING);

    loader_unlock();

    LoaderEvent event = {.type = LoaderEventTypeApplicationStopped};
    furi_pubsub_publish(loader_singleton->pubsub, &event);
}


static int32_t loader_main(void* p) {
    UNUSED(p);

    LoaderMessage message;
    bool exit_requested = false;
    while (!exit_requested) {
        furi_check(loader_singleton != NULL);
        if (furi_message_queue_get(loader_singleton->queue, &message, FuriWaitForever) == FuriStatusOk) {
            FURI_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case LoaderMessageTypeAppStart:
                    // TODO: add bundle
                    message.status_value->value = loader_do_start_by_id(
                        message.start.id,
                        message.start.bundle
                    );
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

static void loader_start(Service service) {
    UNUSED(service);
    furi_check(loader_singleton == NULL);
    loader_singleton = loader_alloc();

    furi_thread_set_priority(loader_singleton->thread, FuriThreadPriorityNormal);
    furi_thread_start(loader_singleton->thread);
}

static void loader_stop(Service service) {
    UNUSED(service);
    furi_check(loader_singleton != NULL);

    // Send stop signal to thread and wait for thread to finish
    LoaderMessage message = {
        .api_lock = NULL,
        .type = LoaderMessageTypeServiceStop
    };
    furi_message_queue_put(loader_singleton->queue, &message, FuriWaitForever);
    furi_thread_join(loader_singleton->thread);

    loader_free();
    loader_singleton = NULL;
}

const ServiceManifest loader_service = {
    .id = "loader",
    .on_start = &loader_start,
    .on_stop = &loader_stop
};

// endregion