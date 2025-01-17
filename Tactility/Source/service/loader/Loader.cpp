#include "app/AppManifest.h"
#include "app/ManifestRegistry.h"
#include "service/ServiceManifest.h"
#include "service/gui/Gui.h"
#include "service/loader/Loader_i.h"
#include "RtosCompat.h"

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "TactilityHeadless.h"

#else
#include "lvgl/LvglSync.h"
#endif

namespace tt::service::loader {

#define TAG "loader"

enum class LoaderStatus {
    Ok,
    ErrorAppStarted,
    ErrorUnknownApp,
    ErrorInternal,
};

typedef struct {
    LoaderEventType type;
} LoaderEventInternal;

// Forward declarations
static void onStartAppMessage(std::shared_ptr<void> message);
static void onStopAppMessage(TT_UNUSED std::shared_ptr<void> message);
static void stopAppInternal();
static LoaderStatus startAppInternal(const std::string& id, std::shared_ptr<const Bundle> _Nullable parameters);

static Loader* loader_singleton = nullptr;

static Loader* loader_alloc() {
    assert(loader_singleton == nullptr);
    loader_singleton = new Loader();
    return loader_singleton;
}

static void loader_free() {
    tt_assert(loader_singleton != nullptr);
    delete loader_singleton;
    loader_singleton = nullptr;
}

void startApp(const std::string& id, const std::shared_ptr<const Bundle>& parameters) {
    TT_LOG_I(TAG, "Start app %s", id.c_str());
    tt_assert(loader_singleton);
    auto message = std::make_shared<LoaderMessageAppStart>(id, parameters);
    loader_singleton->dispatcherThread->dispatch(onStartAppMessage, message);
}

void stopApp() {
    TT_LOG_I(TAG, "Stop app");
    tt_check(loader_singleton);
    loader_singleton->dispatcherThread->dispatch(onStopAppMessage, nullptr);
}

app::AppContext* _Nullable getCurrentApp() {
    tt_assert(loader_singleton);
    if (loader_singleton->mutex.lock(10 / portTICK_PERIOD_MS)) {
        app::AppInstance* app = loader_singleton->appStack.top();
        loader_singleton->mutex.unlock();
        return dynamic_cast<app::AppContext*>(app);
    } else {
        return nullptr;
    }
}

std::shared_ptr<PubSub> getPubsub() {
    tt_assert(loader_singleton);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader_singleton->pubsubExternal;
}

static const char* appStateToString(app::State state) {
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

static void transitionAppToState(app::AppInstance& app, app::State state) {
    const app::AppManifest& manifest = app.getManifest();
    const app::State old_state = app.getState();

    TT_LOG_I(
        TAG,
        "App \"%s\" state: %s -> %s",
        manifest.id.c_str(),
        appStateToString(old_state),
        appStateToString(state)
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
            tt_pubsub_publish(loader_singleton->pubsubExternal, &event_showing);
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
            tt_pubsub_publish(loader_singleton->pubsubExternal, &event_hiding);
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

static LoaderStatus startAppWithManifestInternal(
    const app::AppManifest* manifest,
    std::shared_ptr<const Bundle> _Nullable parameters
) {
    tt_check(loader_singleton != nullptr);

    TT_LOG_I(TAG, "Start with manifest %s", manifest->id.c_str());

    auto scoped_lock = loader_singleton->mutex.scoped();
    if (!scoped_lock->lock(50 / portTICK_PERIOD_MS)) {
        return LoaderStatus::ErrorInternal;
    }

    auto previous_app = !loader_singleton->appStack.empty() ? loader_singleton->appStack.top() : nullptr;
    auto new_app = new app::AppInstance(*manifest, parameters);
    new_app->mutableFlags().showStatusbar = (manifest->type != app::TypeBoot);

    loader_singleton->appStack.push(new_app);
    transitionAppToState(*new_app, app::StateInitial);
    transitionAppToState(*new_app, app::StateStarted);

    // We might have to hide the previous app first
    if (previous_app != nullptr) {
        transitionAppToState(*previous_app, app::StateHiding);
    }

    transitionAppToState(*new_app, app::StateShowing);

    LoaderEventInternal event_internal = {.type = LoaderEventTypeApplicationStarted};
    tt_pubsub_publish(loader_singleton->pubsubInternal, &event_internal);

    LoaderEvent event_external = {
        .type = LoaderEventTypeApplicationStarted,
        .app_started = {
            .app = *new_app
        }
    };
    tt_pubsub_publish(loader_singleton->pubsubExternal, &event_external);

    return LoaderStatus::Ok;
}

static void onStartAppMessage(std::shared_ptr<void> message) {
    auto start_message = std::reinterpret_pointer_cast<LoaderMessageAppStart>(message);
    startAppInternal(start_message->id, start_message->parameters);
}

static void onStopAppMessage(TT_UNUSED std::shared_ptr<void> message) {
    stopAppInternal();
}

static LoaderStatus startAppInternal(
    const std::string& id,
    std::shared_ptr<const Bundle> _Nullable parameters
) {
    TT_LOG_I(TAG, "Start by id %s", id.c_str());

    const app::AppManifest* manifest = app::findAppById(id);
    if (manifest == nullptr) {
        TT_LOG_E(TAG, "App not found: %s", id.c_str());
        return LoaderStatus::ErrorUnknownApp;
    } else {
        return startAppWithManifestInternal(manifest, parameters);
    }
}

static void stopAppInternal() {
    tt_check(loader_singleton != nullptr);

    auto scoped_lock = loader_singleton->mutex.scoped();
    if (!scoped_lock->lock(50 / portTICK_PERIOD_MS)) {
        return;
    }

    size_t original_stack_size = loader_singleton->appStack.size();

    if (original_stack_size == 0) {
        TT_LOG_E(TAG, "Stop app: no app running");
        return;
    }

    // Stop current app
    app::AppInstance* app_to_stop = loader_singleton->appStack.top();

    if (original_stack_size == 1 && app_to_stop->getManifest().type != app::TypeBoot) {
        TT_LOG_E(TAG, "Stop app: can't stop root app");
        return;
    }

    auto result_holder = std::move(app_to_stop->getResult());

    const app::AppManifest& manifest = app_to_stop->getManifest();
    transitionAppToState(*app_to_stop, app::StateHiding);
    transitionAppToState(*app_to_stop, app::StateStopped);

    loader_singleton->appStack.pop();
    delete app_to_stop;

#ifdef ESP_PLATFORM
    TT_LOG_I(TAG, "Free heap: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

    app::AppOnResult on_result = nullptr;
    app::AppInstance* app_to_resume = nullptr;
    // If there's a previous app, resume it
    if (!loader_singleton->appStack.empty()) {
        app_to_resume = loader_singleton->appStack.top();
        tt_assert(app_to_resume);
        transitionAppToState(*app_to_resume, app::StateShowing);

        on_result = app_to_resume->getManifest().onResult;
    }

    // Unlock so that we can send results to app and they can also start/stop new apps while processing these results
    scoped_lock->unlock();
    // WARNING: After this point we cannot change the app states from this method directly anymore as we don't have a lock!

    LoaderEventInternal event_internal = {.type = LoaderEventTypeApplicationStopped};
    tt_pubsub_publish(loader_singleton->pubsubInternal, &event_internal);

    LoaderEvent event_external = {
        .type = LoaderEventTypeApplicationStopped,
        .app_stopped = {
            .manifest = manifest
        }
    };
    tt_pubsub_publish(loader_singleton->pubsubExternal, &event_external);

    if (on_result != nullptr && app_to_resume != nullptr) {
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

// region AppManifest

static void loader_start(TT_UNUSED ServiceContext& service) {
    tt_check(loader_singleton == nullptr);
    loader_singleton = loader_alloc();
    loader_singleton->dispatcherThread->start();
}

static void loader_stop(TT_UNUSED ServiceContext& service) {
    tt_check(loader_singleton != nullptr);

    // Send stop signal to thread and wait for thread to finish
    if (!loader_singleton->mutex.lock(2000 / portTICK_PERIOD_MS)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "loader_stop");
    }
    loader_singleton->dispatcherThread->stop();

    loader_singleton->mutex.unlock();

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
