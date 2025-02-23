#include "Tactility/app/AppManifest.h"
#include "Tactility/app/ManifestRegistry.h"
#include "Tactility/service/gui/Gui.h"
#include "Tactility/service/loader/Loader_i.h"

#include <Tactility/service/ServiceManifest.h>
#include <Tactility/RtosCompat.h>

#ifdef ESP_PLATFORM
#include "Tactility/service/ServiceRegistry.h"
#include <Tactility/TactilityHeadless.h>
#include <esp_heap_caps.h>

#include <utility>
#else
#include "Tactility/lvgl/LvglSync.h"
#endif

namespace tt::service::loader {

#define TAG "loader"
#define LOADER_TIMEOUT (100 / portTICK_PERIOD_MS)

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

private:

    std::shared_ptr<PubSub> pubsubExternal = std::make_shared<PubSub>();
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::stack<std::shared_ptr<app::AppInstance>> appStack;
    /** The dispatcher thread needs a callstack large enough to accommodate all the dispatched methods.
     * This includes full LVGL redraw via Gui::redraw()
     */
    std::unique_ptr<DispatcherThread> dispatcherThread = std::make_unique<DispatcherThread>("loader_dispatcher", 6144); // Files app requires ~5k

    static void onStartAppMessageCallback(std::shared_ptr<void> message);
    static void onStopAppMessageCallback(std::shared_ptr<void> message);

    void onStartAppMessage(const std::string& id, std::shared_ptr<const Bundle> parameters);
    void onStopAppMessage(const std::string& id);

    void transitionAppToState(const std::shared_ptr<app::AppInstance>& app, app::State state);

public:

    void onStart(TT_UNUSED ServiceContext& service) final {
        dispatcherThread->start();
    }

    void onStop(TT_UNUSED ServiceContext& service) final {
        // Send stop signal to thread and wait for thread to finish
        mutex.withLock([this]() {
            dispatcherThread->stop();
        });
    }

    void startApp(const std::string& id, std::shared_ptr<const Bundle> parameters);
    void stopApp();
    std::shared_ptr<app::AppContext> _Nullable getCurrentAppContext();

    std::shared_ptr<PubSub> getPubsub() const { return pubsubExternal; }
};

std::shared_ptr<LoaderService> _Nullable optScreenshotService() {
    return service::findServiceById<LoaderService>(manifest.id);
}

void LoaderService::onStartAppMessageCallback(std::shared_ptr<void> message) {
    auto start_message = std::reinterpret_pointer_cast<LoaderMessageAppStart>(message);
    auto& id = start_message->id;
    auto& parameters = start_message->parameters;

    optScreenshotService()->onStartAppMessage(id, parameters);
}

void LoaderService::onStartAppMessage(const std::string& id, std::shared_ptr<const Bundle> parameters) {

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
    auto new_app = std::make_shared<app::AppInstance>(app_manifest, parameters);

    new_app->mutableFlags().showStatusbar = (app_manifest->type != app::Type::Boot);

    appStack.push(new_app);
    transitionAppToState(new_app, app::State::Initial);
    transitionAppToState(new_app, app::State::Started);

    // We might have to hide the previous app first
    if (previous_app != nullptr) {
        transitionAppToState(previous_app, app::State::Hiding);
    }

    transitionAppToState(new_app, app::State::Showing);

    LoaderEvent event_external = { .type = LoaderEventTypeApplicationStarted };
    pubsubExternal->publish(&event_external);
}

void LoaderService::onStopAppMessageCallback(std::shared_ptr<void> message) {
    TT_LOG_I(TAG, "OnStopAppMessageCallback");
    auto stop_message = std::reinterpret_pointer_cast<LoaderMessageAppStop>(message);
    optScreenshotService()->onStopAppMessage(stop_message->id);
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

    if (app_to_stop->getManifest().id != id) {
        TT_LOG_E(TAG, "Stop app: id mismatch (wanted %s but found %s on top of stack)", id.c_str(), app_to_stop->getManifest().id.c_str());
        return;
    }

    if (original_stack_size == 1 && app_to_stop->getManifest().type != app::Type::Boot) {
        TT_LOG_E(TAG, "Stop app: can't stop root app");
        return;
    }

    bool result_set = false;
    app::Result result;
    std::unique_ptr<Bundle> result_bundle;
    if (app_to_stop->getApp()->moveResult(result, result_bundle)) {
        result_set = true;
    }

    transitionAppToState(app_to_stop, app::State::Hiding);
    transitionAppToState(app_to_stop, app::State::Stopped);

    appStack.pop();

    // We only expect the app to be referenced within the current scope
    if (app_to_stop.use_count() > 1) {
        TT_LOG_W(TAG, "Memory leak: Stopped %s, but use count is %ld", app_to_stop->getManifest().id.c_str(), app_to_stop.use_count() - 1);
    }

    // Refcount is expected to be 2: 1 within app_to_stop and 1 within the current scope
    if (app_to_stop->getApp().use_count() > 2) {
        TT_LOG_W(TAG, "Memory leak: Stopped %s, but use count is %ld", app_to_stop->getManifest().id.c_str(), app_to_stop->getApp().use_count() - 2);
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

    LoaderEvent event_external = { .type = LoaderEventTypeApplicationStopped };
    pubsubExternal->publish(&event_external);

    if (instance_to_resume != nullptr) {
        if (result_set) {
            if (result_bundle != nullptr) {
                instance_to_resume->getApp()->onResult(
                    *instance_to_resume,
                    result,
                    std::move(result_bundle)
                );
            } else {
                instance_to_resume->getApp()->onResult(
                    *instance_to_resume,
                    result,
                    nullptr
                );
            }
        } else {
            const Bundle empty_bundle;
            instance_to_resume->getApp()->onResult(
                *instance_to_resume,
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
        app_manifest.id.c_str(),
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
            LoaderEvent event_showing = { .type = LoaderEventTypeApplicationShowing };
            pubsubExternal->publish(&event_showing);
            break;
        }
        case Hiding: {
            LoaderEvent event_hiding = { .type = LoaderEventTypeApplicationHiding };
            pubsubExternal->publish(&event_hiding);
            break;
        }
        case Stopped:
            // TODO: Verify manifest
            app->getApp()->onDestroy(*app);
            break;
    }

    app->setState(state);
}

void LoaderService::startApp(const std::string& id, std::shared_ptr<const Bundle> parameters) {
    auto message = std::make_shared<LoaderMessageAppStart>(id, std::move(parameters));
    dispatcherThread->dispatch(onStartAppMessageCallback, message);
}

void LoaderService::stopApp() {
    TT_LOG_I(TAG, "stopApp()");
    auto id = getCurrentAppContext()->getManifest().id;
    auto message = std::make_shared<LoaderMessageAppStop>(id);
    dispatcherThread->dispatch(onStopAppMessageCallback, message);
    TT_LOG_I(TAG, "dispatched");
}

std::shared_ptr<app::AppContext> _Nullable LoaderService::getCurrentAppContext() {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return appStack.top();
}

// region Public API

void startApp(const std::string& id, std::shared_ptr<const Bundle> parameters) {
    TT_LOG_I(TAG, "Start app %s", id.c_str());
    auto service = optScreenshotService();
    assert(service);
    service->startApp(id, std::move(parameters));
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

std::shared_ptr<PubSub> getPubsub() {
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
