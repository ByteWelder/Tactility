#include "app_i.h"
#include "app_manifest.h"
#include "app_manifest_registry.h"
#include "loader_i.h"
#include "service_manifest.h"
#include "services/gui/gui.h"
#include <sys/cdefs.h>

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#define TAG "loader"

typedef struct {
    LoaderEventType type;
} LoaderEventInternal;

// Forward declarations
static int32_t loader_main(void* p);

static Loader* loader_singleton = NULL;

static Loader* loader_alloc() {
    tt_check(loader_singleton == NULL);
    loader_singleton = malloc(sizeof(Loader));
    loader_singleton->pubsub_internal = tt_pubsub_alloc();
    loader_singleton->pubsub_external = tt_pubsub_alloc();
    loader_singleton->queue = tt_message_queue_alloc(1, sizeof(LoaderMessage));
    loader_singleton->thread = tt_thread_alloc_ex(
        "loader",
        4096, // Last known minimum was 2400 for starting Hello World app
        &loader_main,
        NULL
    );
    loader_singleton->mutex = tt_mutex_alloc(MutexTypeRecursive);
    loader_singleton->app_stack_index = -1;
    memset(loader_singleton->app_stack, 0, sizeof(App) * APP_STACK_SIZE);
    return loader_singleton;
}

static void loader_free() {
    tt_check(loader_singleton != NULL);
    tt_thread_free(loader_singleton->thread);
    tt_pubsub_free(loader_singleton->pubsub_internal);
    tt_pubsub_free(loader_singleton->pubsub_external);
    tt_message_queue_free(loader_singleton->queue);
    tt_mutex_free(loader_singleton->mutex);
    free(loader_singleton);
}

void loader_lock() {
    tt_assert(loader_singleton);
    tt_assert(loader_singleton->mutex);
    tt_check(tt_mutex_acquire(loader_singleton->mutex, TtWaitForever) == TtStatusOk);
}

void loader_unlock() {
    tt_assert(loader_singleton);
    tt_assert(loader_singleton->mutex);
    tt_check(tt_mutex_release(loader_singleton->mutex) == TtStatusOk);
}

LoaderStatus loader_start_app(const char* id, bool blocking, Bundle* _Nullable bundle) {
    tt_check(loader_singleton);
    LoaderMessageLoaderStatusResult result = {
        .value = LoaderStatusOk
    };

    LoaderMessage message = {
        .type = LoaderMessageTypeAppStart,
        .start.id = id,
        .start.bundle = bundle,
        .status_value = &result,
        .api_lock = blocking ? tt_api_lock_alloc_locked() : NULL
    };

    tt_message_queue_put(loader_singleton->queue, &message, TtWaitForever);

    if (blocking) {
        tt_api_lock_wait_unlock_and_free(message.api_lock);
    }

    return result.value;
}

void loader_stop_app() {
    tt_check(loader_singleton);
    LoaderMessage message = {.type = LoaderMessageTypeAppStop};
    tt_message_queue_put(loader_singleton->queue, &message, TtWaitForever);
}

App _Nullable loader_get_current_app() {
    tt_check(loader_singleton);
    loader_lock();
    App app = (loader_singleton->app_stack_index >= 0)
        ? loader_singleton->app_stack[loader_singleton->app_stack_index]
        : NULL;
    loader_unlock();
    return app;
}

PubSub* loader_get_pubsub() {
    tt_assert(loader_singleton);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader_singleton->pubsub_external;
}

static const char* app_state_to_string(AppState state) {
    switch (state) {
        case AppStateInitial:
            return "initial";
        case AppStateStarted:
            return "started";
        case AppStateShowing:
            return "showing";
        case AppStateHiding:
            return "hiding";
        case AppStateStopped:
            return "stopped";
        default:
            return "?";
    }
}

static void app_transition_to_state(App app, AppState state) {
    const AppManifest* manifest = tt_app_get_manifest(app);
    const AppState old_state = tt_app_get_state(app);

    TT_LOG_I(
        TAG,
        "app \"%s\" state: %s -> %s",
        manifest->id,
        app_state_to_string(old_state),
        app_state_to_string(state)
    );

    switch (state) {
        case AppStateInitial:
            tt_app_set_state(app, AppStateInitial);
            break;
        case AppStateStarted:
            if (manifest->on_start != NULL) {
                manifest->on_start(app);
            }
            tt_app_set_state(app, AppStateStarted);
            break;
        case AppStateShowing:
            LoaderEvent event_showing = {
                .type = LoaderEventTypeApplicationShowing,
                .app_showing = {
                    .app = app
                }
            };
            tt_pubsub_publish(loader_singleton->pubsub_external, &event_showing);
            tt_app_set_state(app, AppStateShowing);
            break;
        case AppStateHiding:
            LoaderEvent event_hiding = {
                .type = LoaderEventTypeApplicationHiding,
                .app_hiding = {
                    .app = app
                }
            };
            tt_pubsub_publish(loader_singleton->pubsub_external, &event_hiding);
            tt_app_set_state(app, AppStateHiding);
            break;
        case AppStateStopped:
            if (manifest->on_stop) {
                manifest->on_stop(app);
            }
            tt_app_set_data(app, NULL);
            tt_app_set_state(app, AppStateStopped);
            break;
    }
}

LoaderStatus loader_do_start_app_with_manifest(
    const AppManifest* manifest,
    Bundle* _Nullable bundle
) {
    TT_LOG_I(TAG, "start with manifest %s", manifest->id);

    loader_lock();

    if (loader_singleton->app_stack_index >= (APP_STACK_SIZE - 1)) {
        TT_LOG_E(TAG, "failed to start app: stack limit of %d reached", APP_STACK_SIZE);
        return LoaderStatusErrorInternal;
    }

    int8_t previous_index = loader_singleton->app_stack_index;
    loader_singleton->app_stack_index++;

    App app = tt_app_alloc(manifest, bundle);
    tt_assert(loader_singleton->app_stack[loader_singleton->app_stack_index] == NULL);
    loader_singleton->app_stack[loader_singleton->app_stack_index] = app;
    app_transition_to_state(app, AppStateInitial);
    app_transition_to_state(app, AppStateStarted);

    // We might have to hide the previous app first
    if (previous_index != -1) {
        App previous_app = loader_singleton->app_stack[previous_index];
        app_transition_to_state(previous_app, AppStateHiding);
    }

    app_transition_to_state(app, AppStateShowing);

    loader_unlock();

    LoaderEventInternal event_internal = {.type = LoaderEventTypeApplicationStarted};
    tt_pubsub_publish(loader_singleton->pubsub_internal, &event_internal);

    LoaderEvent event_external = {
        .type = LoaderEventTypeApplicationStarted,
        .app_started = {
            .app = app
        }
    };
    tt_pubsub_publish(loader_singleton->pubsub_external, &event_external);

    return LoaderStatusOk;
}

static LoaderStatus loader_do_start_by_id(
    const char* id,
    Bundle* _Nullable bundle
) {
    TT_LOG_I(TAG, "Start by id %s", id);

    const AppManifest* manifest = tt_app_manifest_registry_find_by_id(id);
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
        TT_LOG_E(TAG, "Stop app: no app running");
        return;
    }

    if (current_app_index == 0) {
        loader_unlock();
        TT_LOG_E(TAG, "Stop app: can't stop root app");
        return;
    }

    // Stop current app
    App app_to_stop = loader_singleton->app_stack[current_app_index];
    const AppManifest* manifest = tt_app_get_manifest(app_to_stop);
    app_transition_to_state(app_to_stop, AppStateHiding);
    app_transition_to_state(app_to_stop, AppStateStopped);

    tt_app_free(app_to_stop);
    loader_singleton->app_stack[current_app_index] = NULL;
    loader_singleton->app_stack_index--;

#ifdef ESP_PLATFORM
    TT_LOG_I(TAG, "Free heap: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

    // Resume previous app
    tt_assert(loader_singleton->app_stack[loader_singleton->app_stack_index] != NULL);
    App app_to_resume = loader_singleton->app_stack[loader_singleton->app_stack_index];
    app_transition_to_state(app_to_resume, AppStateShowing);

    loader_unlock();

    LoaderEventInternal event_internal = {.type = LoaderEventTypeApplicationStopped};
    tt_pubsub_publish(loader_singleton->pubsub_internal, &event_internal);

    LoaderEvent event_external = {
        .type = LoaderEventTypeApplicationStopped,
        .app_stopped = {
            .manifest = manifest
        }
    };
    tt_pubsub_publish(loader_singleton->pubsub_external, &event_external);
}


static int32_t loader_main(TT_UNUSED void* parameter) {
    LoaderMessage message;
    bool exit_requested = false;
    while (!exit_requested) {
        tt_check(loader_singleton != NULL);
        if (tt_message_queue_get(loader_singleton->queue, &message, TtWaitForever) == TtStatusOk) {
            TT_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case LoaderMessageTypeAppStart:
                    message.status_value->value = loader_do_start_by_id(
                        message.start.id,
                        message.start.bundle
                    );
                    if (message.api_lock) {
                        tt_api_lock_unlock(message.api_lock);
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

static void loader_start(TT_UNUSED Service service) {
    tt_check(loader_singleton == NULL);
    loader_singleton = loader_alloc();

    tt_thread_set_priority(loader_singleton->thread, THREAD_PRIORITY_SERVICE);
    tt_thread_start(loader_singleton->thread);
}

static void loader_stop(TT_UNUSED Service service) {
    tt_check(loader_singleton != NULL);

    // Send stop signal to thread and wait for thread to finish
    LoaderMessage message = {
        .api_lock = NULL,
        .type = LoaderMessageTypeServiceStop
    };
    tt_message_queue_put(loader_singleton->queue, &message, TtWaitForever);
    tt_thread_join(loader_singleton->thread);
    tt_thread_free(loader_singleton->thread);

    loader_free();
    loader_singleton = NULL;
}

const ServiceManifest loader_service = {
    .id = "loader",
    .on_start = &loader_start,
    .on_stop = &loader_stop
};

// endregion
