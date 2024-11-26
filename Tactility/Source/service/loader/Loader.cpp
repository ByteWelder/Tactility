#include "Tactility.h"
#include "app/Manifest.h"
#include "app/ManifestRegistry.h"
#include "service/Manifest.h"
#include "service/gui/Gui.h"
#include "service/loader/Loader_i.h"

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#include "ui/LvglSync.h"

#endif

namespace tt::service::loader {

#define TAG "loader"

typedef struct {
    LoaderEventType type;
} LoaderEventInternal;

// Forward declarations
static int32_t loader_main(void* p);

static Loader* loader_singleton = nullptr;

static Loader* loader_alloc() {
    assert(loader_singleton == nullptr);
    loader_singleton = new Loader();
    loader_singleton->pubsub_internal = tt_pubsub_alloc();
    loader_singleton->pubsub_external = tt_pubsub_alloc();
    loader_singleton->thread = new Thread(
        "loader",
        4096, // Last known minimum was 2400 for starting Hello World app
        &loader_main,
        nullptr
    );
    loader_singleton->mutex = tt_mutex_alloc(MutexTypeRecursive);
    loader_singleton->app_stack_index = -1;
    memset(loader_singleton->app_stack, 0, sizeof(app::App) * APP_STACK_SIZE);
    return loader_singleton;
}

static void loader_free() {
    tt_assert(loader_singleton != nullptr);
    delete loader_singleton->thread;
    tt_pubsub_free(loader_singleton->pubsub_internal);
    tt_pubsub_free(loader_singleton->pubsub_external);
    tt_mutex_free(loader_singleton->mutex);
    delete loader_singleton;
    loader_singleton = nullptr;
}

static void loader_lock() {
    tt_assert(loader_singleton);
    tt_assert(loader_singleton->mutex);
    tt_check(tt_mutex_acquire(loader_singleton->mutex, TtWaitForever) == TtStatusOk);
}

static void loader_unlock() {
    tt_assert(loader_singleton);
    tt_assert(loader_singleton->mutex);
    tt_check(tt_mutex_release(loader_singleton->mutex) == TtStatusOk);
}

LoaderStatus start_app(const std::string& id, bool blocking, const Bundle& bundle) {
    tt_assert(loader_singleton);

    LoaderMessageLoaderStatusResult result = {
        .value = LoaderStatusOk
    };

    auto* start_message = new LoaderMessageAppStart(id, bundle);
    LoaderMessage message(start_message, result);

    EventFlag* event_flag = blocking ? new EventFlag() : nullptr;
    if (event_flag != nullptr) {
        message.setApiLock(event_flag);
    }

    loader_singleton->queue.put(&message, TtWaitForever);

    if (event_flag != nullptr) {
        /* TODO: Check if task id is not the LVGL one,
         because otherwise this fails as the apps starting logic will try to lock lvgl
         to update the UI and fail. */
        event_flag->wait(TT_API_LOCK_EVENT);
        delete event_flag;
    }

    return result.value;
}

void stop_app() {
    tt_check(loader_singleton);
    LoaderMessage message(LoaderMessageTypeAppStop);
    loader_singleton->queue.put(&message, TtWaitForever);
}

app::App _Nullable get_current_app() {
    tt_assert(loader_singleton);
    loader_lock();
    app::App app = (loader_singleton->app_stack_index >= 0)
        ? loader_singleton->app_stack[loader_singleton->app_stack_index]
        : nullptr;
    loader_unlock();
    return app;
}

PubSub* get_pubsub() {
    tt_assert(loader_singleton);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader_singleton->pubsub_external;
}

static const char* app_state_to_string(app::State state) {
    switch (state) {
        case app::StateInitial:
            return "initial";
        case app::StateStarted:
            return "started";
        case app::StateShowing:
            return "showing";
        case app::StateHiding:
            return "hiding";
        case app::StateStopped:
            return "stopped";
        default:
            return "?";
    }
}

static void app_transition_to_state(app::App app, app::State state) {
    const app::Manifest& manifest = app::tt_app_get_manifest(app);
    const app::State old_state = app::tt_app_get_state(app);

    TT_LOG_I(
        TAG,
        "app \"%s\" state: %s -> %s",
        manifest.id.c_str(),
        app_state_to_string(old_state),
        app_state_to_string(state)
    );

    switch (state) {
        case app::StateInitial:
            tt_app_set_state(app, app::StateInitial);
            break;
        case app::StateStarted:
            if (manifest.on_start != nullptr) {
                manifest.on_start(app);
            }
            tt_app_set_state(app, app::StateStarted);
            break;
        case app::StateShowing: {
            LoaderEvent event_showing = {
                .type = LoaderEventTypeApplicationShowing,
                .app_showing = {
                    .app = static_cast<app::App*>(app)
                }
            };
            tt_pubsub_publish(loader_singleton->pubsub_external, &event_showing);
            tt_app_set_state(app, app::StateShowing);
            break;
        }
        case app::StateHiding: {
            LoaderEvent event_hiding = {
                .type = LoaderEventTypeApplicationHiding,
                .app_hiding = {
                    .app = static_cast<app::App*>(app)
                }
            };
            tt_pubsub_publish(loader_singleton->pubsub_external, &event_hiding);
            tt_app_set_state(app, app::StateHiding);
            break;
        }
        case app::StateStopped:
            if (manifest.on_stop) {
                manifest.on_stop(app);
            }
            app::tt_app_set_data(app, nullptr);
            tt_app_set_state(app, app::StateStopped);
            break;
    }
}

static LoaderStatus loader_do_start_app_with_manifest(
    const app::Manifest* manifest,
    const Bundle& bundle
) {
    TT_LOG_I(TAG, "start with manifest %s", manifest->id.c_str());

    loader_lock();

    if (loader_singleton->app_stack_index >= (APP_STACK_SIZE - 1)) {
        TT_LOG_E(TAG, "failed to start app: stack limit of %d reached", APP_STACK_SIZE);
        return LoaderStatusErrorInternal;
    }

    int8_t previous_index = loader_singleton->app_stack_index;
    loader_singleton->app_stack_index++;

    app::App app = tt_app_alloc(*manifest, bundle);
    tt_check(loader_singleton->app_stack[loader_singleton->app_stack_index] == nullptr);
    loader_singleton->app_stack[loader_singleton->app_stack_index] = app;
    app_transition_to_state(app, app::StateInitial);
    app_transition_to_state(app, app::StateStarted);

    // We might have to hide the previous app first
    if (previous_index != -1) {
        app::App previous_app = loader_singleton->app_stack[previous_index];
        app_transition_to_state(previous_app, app::StateHiding);
    }

    app_transition_to_state(app, app::StateShowing);

    loader_unlock();

    LoaderEventInternal event_internal = {.type = LoaderEventTypeApplicationStarted};
    tt_pubsub_publish(loader_singleton->pubsub_internal, &event_internal);

    LoaderEvent event_external = {
        .type = LoaderEventTypeApplicationStarted,
        .app_started = {
            .app = static_cast<app::App*>(app)
        }
    };
    tt_pubsub_publish(loader_singleton->pubsub_external, &event_external);

    return LoaderStatusOk;
}

static LoaderStatus do_start_by_id(
    const std::string& id,
    const Bundle& bundle
) {
    TT_LOG_I(TAG, "Start by id %s", id.c_str());

    const app::Manifest* manifest = app::app_manifest_registry_find_by_id(id);
    if (manifest == nullptr) {
        return LoaderStatusErrorUnknownApp;
    } else {
        return loader_do_start_app_with_manifest(manifest, bundle);
    }
}


static void do_stop_app() {
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
    app::App app_to_stop = loader_singleton->app_stack[current_app_index];
    const app::Manifest& manifest = app::tt_app_get_manifest(app_to_stop);
    app_transition_to_state(app_to_stop, app::StateHiding);
    app_transition_to_state(app_to_stop, app::StateStopped);

    app::tt_app_free(app_to_stop);
    loader_singleton->app_stack[current_app_index] = nullptr;
    loader_singleton->app_stack_index--;

#ifdef ESP_PLATFORM
    TT_LOG_I(TAG, "Free heap: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

    // Resume previous app
    tt_assert(loader_singleton->app_stack[loader_singleton->app_stack_index] != nullptr);
    app::App app_to_resume = loader_singleton->app_stack[loader_singleton->app_stack_index];
    app_transition_to_state(app_to_resume, app::StateShowing);

    loader_unlock();

    LoaderEventInternal event_internal = {.type = LoaderEventTypeApplicationStopped};
    tt_pubsub_publish(loader_singleton->pubsub_internal, &event_internal);

    LoaderEvent event_external = {
        .type = LoaderEventTypeApplicationStopped,
        .app_stopped = {
            .manifest = &manifest
        }
    };
    tt_pubsub_publish(loader_singleton->pubsub_external, &event_external);
}


static int32_t loader_main(TT_UNUSED void* parameter) {
    LoaderMessage message;
    bool exit_requested = false;
    while (!exit_requested) {
        tt_assert(loader_singleton != nullptr);
        if (loader_singleton->queue.get(&message, TtWaitForever) == TtStatusOk) {
            TT_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case LoaderMessageTypeAppStart:
                    message.result.status_value.value = do_start_by_id(
                        message.payload.start->id,
                        message.payload.start->bundle
                    );
                    if (message.api_lock != nullptr) {
                        message.api_lock->set(TT_API_LOCK_EVENT);
                    }
                    message.cleanup();
                    break;
                case LoaderMessageTypeAppStop:
                    do_stop_app();
                    break;
                case LoaderMessageTypeServiceStop:
                    exit_requested = true;
                    break;
                case LoaderMessageTypeNone:
                    break;
            }
        }
    }

    return 0;
}

// region AppManifest

static void loader_start(TT_UNUSED Service& service) {
    tt_check(loader_singleton == nullptr);
    loader_singleton = loader_alloc();

    loader_singleton->thread->setPriority(THREAD_PRIORITY_SERVICE);
    loader_singleton->thread->start();
}

static void loader_stop(TT_UNUSED Service& service) {
    tt_check(loader_singleton != nullptr);

    // Send stop signal to thread and wait for thread to finish
    loader_lock();
    LoaderMessage message(LoaderMessageTypeServiceStop);
    loader_singleton->queue.put(&message, TtWaitForever);
    loader_unlock();

    loader_singleton->thread->join();
    delete loader_singleton->thread;

    loader_free();
    loader_singleton = nullptr;
}

extern const Manifest manifest = {
    .id = "Loader",
    .onStart = &loader_start,
    .onStop = &loader_stop
};

// endregion

} // namespace
