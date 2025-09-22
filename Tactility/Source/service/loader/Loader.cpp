#include "Tactility/service/loader/Loader.h"
#include "Tactility/app/AppInstance.h"
#include "Tactility/app/AppManifest.h"
#include "Tactility/app/AppRegistration.h"

#include <Tactility/DispatcherThread.h>
#include <Tactility/service/ServiceManifest.h>
#include <Tactility/service/ServiceRegistration.h>

#include <stack>

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#include <utility>
#else
#include "Tactility/lvgl/LvglSync.h"
#endif

namespace tt::service::loader {

constexpr auto* TAG = "Loader";
constexpr auto LOADER_TIMEOUT = (100 / portTICK_PERIOD_MS);

extern const ServiceManifest manifest;

static const char* appStateToString(app::State state) {
    switch (state) {
        using enum app::State;
        case Initial:
            return "initial";
        case Started:
            return "started";
        case Showing:
            return "showing";
        case Hiding:
            return "hiding";
        case Stopped:
            return "stopped";
        default:
            return "?";
    }
}

// region AppManifest

class LoaderService final : public Service {

    std::shared_ptr<PubSub<LoaderEvent>> pubsubExternal = std::make_shared<PubSub<LoaderEvent>>();
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::stack<std::shared_ptr<app::AppInstance>> appStack;
    app::LaunchId nextLaunchId = 0;

    /** The dispatcher thread needs a callstack large enough to accommodate all the dispatched methods.
     * This includes full LVGL redraw via Gui::redraw()
     */
    std::unique_ptr<DispatcherThread> dispatcherThread = std::make_unique<DispatcherThread>("loader_dispatcher", 6144); // Files app requires ~5k

    void onStartAppMessage(const std::string& id, app::LaunchId launchId, std::shared_ptr<const Bundle> parameters);
    void onStopAppMessage(const std::string& id);

    void transitionAppToState(const std::shared_ptr<app::AppInstance>& app, app::State state);

public:

    bool onStart(TT_UNUSED ServiceContext& service) override {
        dispatcherThread->start();
        return true;
    }

    void onStop(TT_UNUSED ServiceContext& service) override {
        // Send stop signal to thread and wait for thread to finish
        mutex.withLock([this] {
            dispatcherThread->stop();
        });
    }

    app::LaunchId startApp(const std::string& id, std::shared_ptr<const Bundle> parameters);
    void stopApp();
    std::shared_ptr<app::AppContext> _Nullable getCurrentAppContext();

    std::shared_ptr<PubSub<LoaderEvent>> getPubsub() const { return pubsubExternal; }
};

std::shared_ptr<LoaderService> _Nullable optScreenshotService() {
    return service::findServiceById<LoaderService>(manifest.id);
}

void LoaderService::onStartAppMessage(const std::string& id, app::LaunchId launchId, std::shared_ptr<const Bundle> parameters) {
    TT_LOG_I(TAG, "Start by id %s", id.c_str());

    auto app_manifest = app::findAppById(id);
    if (app_manifest == nullptr) {
        TT_LOG_E(TAG, "App not found: %s", id.c_str());
        return;
    }

    auto lock = mutex.asScopedLock();
    if (!lock.lock(LOADER_TIMEOUT)) {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    auto previous_app = !appStack.empty() ? appStack.top() : nullptr;
    auto new_app = std::make_shared<app::AppInstance>(app_manifest, launchId, parameters);

    new_app->mutableFlags().hideStatusbar = (app_manifest->appFlags & app::AppManifest::Flags::HideStatusBar);

    appStack.push(new_app);
    transitionAppToState(new_app, app::State::Initial);
    transitionAppToState(new_app, app::State::Started);

    // We might have to hide the previous app first
    if (previous_app != nullptr) {
        transitionAppToState(previous_app, app::State::Hiding);
    }

    transitionAppToState(new_app, app::State::Showing);

    pubsubExternal->publish(LoaderEvent::ApplicationStarted);
}

void LoaderService::onStopAppMessage(const std::string& id) {
    auto lock = mutex.asScopedLock();
    if (!lock.lock(LOADER_TIMEOUT)) {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    size_t original_stack_size = appStack.size();

    if (original_stack_size == 0) {
        TT_LOG_E(TAG, "Stop app: no app running");
        return;
    }

    // Stop current app
    auto app_to_stop = appStack.top();

    if (app_to_stop->getManifest().appId != id) {
        TT_LOG_E(TAG, "Stop app: id mismatch (wanted %s but found %s on top of stack)", id.c_str(), app_to_stop->getManifest().appId.c_str());
        return;
    }

    if (original_stack_size == 1 && app_to_stop->getManifest().appName != "Boot") {
        TT_LOG_E(TAG, "Stop app: can't stop root app");
        return;
    }

    bool result_set = false;
    app::Result result;
    std::unique_ptr<Bundle> result_bundle;
    if (app_to_stop->getApp()->moveResult(result, result_bundle)) {
        result_set = true;
    }

    auto app_to_stop_launch_id = app_to_stop->getLaunchId();

    transitionAppToState(app_to_stop, app::State::Hiding);
    transitionAppToState(app_to_stop, app::State::Stopped);

    appStack.pop();

    // We only expect the app to be referenced within the current scope
    if (app_to_stop.use_count() > 1) {
        TT_LOG_W(TAG, "Memory leak: Stopped %s, but use count is %ld", app_to_stop->getManifest().appId.c_str(), app_to_stop.use_count() - 1);
    }

    // Refcount is expected to be 2: 1 within app_to_stop and 1 within the current scope
    if (app_to_stop->getApp().use_count() > 2) {
        TT_LOG_W(TAG, "Memory leak: Stopped %s, but use count is %ld", app_to_stop->getManifest().appId.c_str(), app_to_stop->getApp().use_count() - 2);
    }

#ifdef ESP_PLATFORM
    TT_LOG_I(TAG, "Free heap: %zu", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

    std::shared_ptr<app::AppInstance> instance_to_resume;
    // If there's a previous app, resume it
    if (!appStack.empty()) {
        instance_to_resume = appStack.top();
        assert(instance_to_resume);
        transitionAppToState(instance_to_resume, app::State::Showing);
    }

    // Unlock so that we can send results to app and they can also start/stop new apps while processing these results
    lock.unlock();
    // WARNING: After this point we cannot change the app states from this method directly anymore as we don't have a lock!

    pubsubExternal->publish(LoaderEvent::ApplicationStopped);

    if (instance_to_resume != nullptr) {
        if (result_set) {
            if (result_bundle != nullptr) {
                instance_to_resume->getApp()->onResult(
                    *instance_to_resume,
                    app_to_stop_launch_id,
                    result,
                    std::move(result_bundle)
                );
            } else {
                instance_to_resume->getApp()->onResult(
                    *instance_to_resume,
                    app_to_stop_launch_id,
                    result,
                    nullptr
                );
            }
        } else {
            const Bundle empty_bundle;
            instance_to_resume->getApp()->onResult(
                *instance_to_resume,
                app_to_stop_launch_id,
                app::Result::Cancelled,
                nullptr
            );
        }
    }
}

void LoaderService::transitionAppToState(const std::shared_ptr<app::AppInstance>& app, app::State state) {
    const app::AppManifest& app_manifest = app->getManifest();
    const app::State old_state = app->getState();

    TT_LOG_I(
        TAG,
        "App \"%s\" state: %s -> %s",
        app_manifest.appId.c_str(),
        appStateToString(old_state),
        appStateToString(state)
    );

    switch (state) {
        using enum app::State;
        case Initial:
            break;
        case Started:
            app->getApp()->onCreate(*app);
            break;
        case Showing: {
            pubsubExternal->publish(LoaderEvent::ApplicationShowing);
            break;
        }
        case Hiding: {
            pubsubExternal->publish(LoaderEvent::ApplicationHiding);
            break;
        }
        case Stopped:
            // TODO: Verify manifest
            app->getApp()->onDestroy(*app);
            break;
    }

    app->setState(state);
}

app::LaunchId LoaderService::startApp(const std::string& id, std::shared_ptr<const Bundle> parameters) {
    auto launch_id = nextLaunchId++;
    dispatcherThread->dispatch([this, id, launch_id, parameters]() {
        onStartAppMessage(id, launch_id, parameters);
    });
    return launch_id;
}

void LoaderService::stopApp() {
    TT_LOG_I(TAG, "stopApp()");
    auto id = getCurrentAppContext()->getManifest().appId;
    dispatcherThread->dispatch([this, id]() {
        onStopAppMessage(id);
    });
    TT_LOG_I(TAG, "dispatched");
}

std::shared_ptr<app::AppContext> _Nullable LoaderService::getCurrentAppContext() {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return appStack.top();
}

// region Public API

app::LaunchId startApp(const std::string& id, std::shared_ptr<const Bundle> parameters) {
    TT_LOG_I(TAG, "Start app %s", id.c_str());
    auto service = optScreenshotService();
    assert(service);
    return service->startApp(id, std::move(parameters));
}

void stopApp() {
    TT_LOG_I(TAG, "Stop app");
    auto service = optScreenshotService();
    service->stopApp();
}

std::shared_ptr<app::AppContext> _Nullable getCurrentAppContext() {
    auto service = optScreenshotService();
    assert(service);
    return service->getCurrentAppContext();
}

std::shared_ptr<app::App> _Nullable getCurrentApp() {
    auto app_context = getCurrentAppContext();
    return app_context != nullptr ? app_context->getApp() : nullptr;
}

std::shared_ptr<PubSub<LoaderEvent>> getPubsub() {
    auto service = optScreenshotService();
    assert(service);
    return service->getPubsub();
}

// endregion Public API

extern const ServiceManifest manifest = {
    .id = "Loader",
    .createService = create<LoaderService>
};

// endregion

} // namespace
