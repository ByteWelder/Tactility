#include "Tactility.h"
#include <Mutex.h>
#include "app/AppManifest.h"
#include "app/ManifestRegistry.h"
#include "service/ServiceManifest.h"
#include "service/gui/Gui.h"
#include "service/loader/Loader_i.h"
#include "RtosCompat.h"

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#else
#include "lvgl/LvglSync.h"
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
    loader_singleton->thread = new Thread(
        "loader",
        4096, // Last known minimum was 2400 for starting Hello World app
        &loader_main,
        nullptr
    );
    return loader_singleton;
}

static void loader_free() {
    tt_assert(loader_singleton != nullptr);
    delete loader_singleton->thread;
    delete loader_singleton;
    loader_singleton = nullptr;
}

static void loader_lock() {
    tt_assert(loader_singleton);
    tt_check(loader_singleton->mutex.acquire(TtWaitForever) == TtStatusOk);
}

static void loader_unlock() {
    tt_assert(loader_singleton);
    tt_check(loader_singleton->mutex.release() == TtStatusOk);
}

LoaderStatus startApp(const std::string& id, bool blocking, std::shared_ptr<const Bundle> parameters) {
    TT_LOG_I(TAG, "Start app %s", id.c_str());
    tt_assert(loader_singleton);

    LoaderMessageLoaderStatusResult result = {
        .value = LoaderStatusOk
    };

    auto* start_message = new LoaderMessageAppStart(id, parameters);
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

void stopApp() {
    TT_LOG_I(TAG, "Stop app");
    tt_check(loader_singleton);
    LoaderMessage message(LoaderMessageTypeAppStop);
    loader_singleton->queue.put(&message, TtWaitForever);
}

app::AppContext* _Nullable getCurrentApp() {
    tt_assert(loader_singleton);
    loader_lock();
    app::AppInstance* app = loader_singleton->app_stack.top();
    loader_unlock();
    return dynamic_cast<app::AppContext*>(app);
}

std::shared_ptr<PubSub> getPubsub() {
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

static void app_transition_to_state(app::AppInstance& app, app::State state) {
    const app::AppManifest& manifest = app.getManifest();
    const app::State old_state = app.getState();

    TT_LOG_I(
        TAG,
        "app \"%s\" state: %s -> %s",
        manifest.id.c_str(),
        app_state_to_string(old_state),
        app_state_to_string(state)
    );

    switch (state) {
        case app::StateInitial:
            app.setState(app::StateInitial);
            break;
        case app::StateStarted:
            if (manifest.onStart != nullptr) {
                manifest.onStart(app);
            }
            app.setState(app::StateStarted);
            break;
        case app::StateShowing: {
            LoaderEvent event_showing = {
                .type = LoaderEventTypeApplicationShowing,
                .app_showing = {
                    .app = app
                }
            };
            tt_pubsub_publish(loader_singleton->pubsub_external, &event_showing);
            app.setState(app::StateShowing);
            break;
        }
        case app::StateHiding: {
            LoaderEvent event_hiding = {
                .type = LoaderEventTypeApplicationHiding,
                .app_hiding = {
                    .app = app
                }
            };
            tt_pubsub_publish(loader_singleton->pubsub_external, &event_hiding);
            app.setState(app::StateHiding);
            break;
        }
        case app::StateStopped:
            if (manifest.onStop) {
                manifest.onStop(app);
            }
            app.setData(nullptr);
            app.setState(app::StateStopped);
            break;
    }
}

static LoaderStatus loader_do_start_app_with_manifest(
    const app::AppManifest* manifest,
    std::shared_ptr<const Bundle> _Nullable parameters
) {
    TT_LOG_I(TAG, "start with manifest %s", manifest->id.c_str());

    loader_lock();

    auto previous_app = !loader_singleton->app_stack.empty() ? loader_singleton->app_stack.top() : nullptr;
    auto new_app = new app::AppInstance(*manifest, parameters);
    new_app->mutableFlags().showStatusbar = (manifest->type != app::TypeBoot);

    loader_singleton->app_stack.push(new_app);
    app_transition_to_state(*new_app, app::StateInitial);
    app_transition_to_state(*new_app, app::StateStarted);

    // We might have to hide the previous app first
    if (previous_app != nullptr) {
        app_transition_to_state(*previous_app, app::StateHiding);
    }

    app_transition_to_state(*new_app, app::StateShowing);

    loader_unlock();

    LoaderEventInternal event_internal = {.type = LoaderEventTypeApplicationStarted};
    tt_pubsub_publish(loader_singleton->pubsub_internal, &event_internal);

    LoaderEvent event_external = {
        .type = LoaderEventTypeApplicationStarted,
        .app_started = {
            .app = *new_app
        }
    };
    tt_pubsub_publish(loader_singleton->pubsub_external, &event_external);

    return LoaderStatusOk;
}

static LoaderStatus do_start_by_id(
    const std::string& id,
    std::shared_ptr<const Bundle> _Nullable parameters
) {
    TT_LOG_I(TAG, "Start by id %s", id.c_str());

    const app::AppManifest* manifest = app::findAppById(id);
    if (manifest == nullptr) {
        TT_LOG_E(TAG, "App not found: %s", id.c_str());
        return LoaderStatusErrorUnknownApp;
    } else {
        return loader_do_start_app_with_manifest(manifest, parameters);
    }
}


static void do_stop_app() {
    loader_lock();

    size_t original_stack_size = loader_singleton->app_stack.size();

    if (original_stack_size == 0) {
        loader_unlock();
        TT_LOG_E(TAG, "Stop app: no app running");
        return;
    }

    // Stop current app
    app::AppInstance* app_to_stop = loader_singleton->app_stack.top();

    if (original_stack_size == 1 && app_to_stop->getManifest().type != app::TypeBoot) {
        loader_unlock();
        TT_LOG_E(TAG, "Stop app: can't stop root app");
        return;
    }

    std::unique_ptr<app::ResultHolder> result_holder = std::move(app_to_stop->getResult());

    const app::AppManifest& manifest = app_to_stop->getManifest();
    app_transition_to_state(*app_to_stop, app::StateHiding);
    app_transition_to_state(*app_to_stop, app::StateStopped);

    loader_singleton->app_stack.pop();
    delete app_to_stop;

#ifdef ESP_PLATFORM
    TT_LOG_I(TAG, "Free heap: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

    // If there's a previous app, resume it
    if (!loader_singleton->app_stack.empty()) {
        app::AppInstance* app_to_resume = loader_singleton->app_stack.top();
        tt_assert(app_to_resume);
        app_transition_to_state(*app_to_resume, app::StateShowing);

        auto on_result = app_to_resume->getManifest().onResult;
        if (on_result != nullptr) {
            if (result_holder != nullptr) {
                auto result_bundle = result_holder->resultData.get();
                if (result_bundle != nullptr) {
                    on_result(
                        *app_to_resume,
                        result_holder->result,
                        *result_bundle
                    );
                } else {
                    const Bundle empty_bundle;
                    on_result(
                        *app_to_resume,
                        result_holder->result,
                        empty_bundle
                    );
                }
            } else {
                const Bundle empty_bundle;
                on_result(
                    *app_to_resume,
                    app::ResultCancelled,
                    empty_bundle
                );
            }
        }
    }

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
        tt_assert(loader_singleton != nullptr);
        if (loader_singleton->queue.get(&message, TtWaitForever) == TtStatusOk) {
            TT_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case LoaderMessageTypeAppStart:
                    message.result.status_value.value = do_start_by_id(
                        message.payload.start->id,
                        message.payload.start->parameters
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

static void loader_start(TT_UNUSED ServiceContext& service) {
    tt_check(loader_singleton == nullptr);
    loader_singleton = loader_alloc();

    loader_singleton->thread->setPriority(THREAD_PRIORITY_SERVICE);
    loader_singleton->thread->start();
}

static void loader_stop(TT_UNUSED ServiceContext& service) {
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

extern const ServiceManifest manifest = {
    .id = "Loader",
    .onStart = &loader_start,
    .onStop = &loader_stop
};

// endregion

} // namespace
