#include "Tactility/app/AppManifest.h"
#include "Tactility/app/ManifestRegistry.h"
#include "Tactility/service/gui/Gui.h"
#include "Tactility/service/loader/Loader_i.h"

#include <Tactility/service/ServiceManifest.h>
#include <Tactility/RtosCompat.h>

#ifdef ESP_PLATFORM
#include "Tactility/app/ElfApp.h"
#include <Tactility/TactilityHeadless.h>
#include <esp_heap_caps.h>
#else
#include "Tactility/lvgl/LvglSync.h"
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
    assert(loader_singleton != nullptr);
    delete loader_singleton;
    loader_singleton = nullptr;
}

void startApp(const std::string& id, std::shared_ptr<const Bundle> parameters) {
    TT_LOG_I(TAG, "Start app %s", id.c_str());
    assert(loader_singleton);
    auto message = std::make_shared<LoaderMessageAppStart>(id, std::move(parameters));
    loader_singleton->dispatcherThread->dispatch(onStartAppMessage, message);
}

void stopApp() {
    TT_LOG_I(TAG, "Stop app");
    tt_check(loader_singleton);
    loader_singleton->dispatcherThread->dispatch(onStopAppMessage, nullptr);
}

std::shared_ptr<app::AppContext> _Nullable getCurrentAppContext() {
    assert(loader_singleton);
    loader_singleton->mutex.lock();
    auto app = loader_singleton->appStack.top();
    loader_singleton->mutex.unlock();
    return app;
}

std::shared_ptr<app::App> _Nullable getCurrentApp() {
    auto app_context = getCurrentAppContext();
    return app_context != nullptr ? app_context->getApp() : nullptr;
}

std::shared_ptr<PubSub> getPubsub() {
    assert(loader_singleton);
    // it's safe to return pubsub without locking
    // because it's never freed and loader is never exited
    // also the loader instance cannot be obtained until the pubsub is created
    return loader_singleton->pubsubExternal;
}

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

static void transitionAppToState(std::shared_ptr<app::AppInstance> app, app::State state) {
    const app::AppManifest& manifest = app->getManifest();
    const app::State old_state = app->getState();

    TT_LOG_I(
        TAG,
        "App \"%s\" state: %s -> %s",
        manifest.id.c_str(),
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
            loader_singleton->pubsubExternal->publish(&event_showing);
            break;
        }
        case Hiding: {
            LoaderEvent event_hiding = { .type = LoaderEventTypeApplicationHiding };
            loader_singleton->pubsubExternal->publish(&event_hiding);
            break;
        }
        case Stopped:
            // TODO: Verify manifest
            app->getApp()->onDestroy(*app);
            break;
    }

    app->setState(state);
}

static LoaderStatus startAppWithManifestInternal(
    const std::shared_ptr<app::AppManifest>& manifest,
    const std::shared_ptr<const Bundle> _Nullable& parameters
) {
    tt_check(loader_singleton != nullptr);

    TT_LOG_I(TAG, "Start with manifest %s", manifest->id.c_str());

    auto lock = loader_singleton->mutex.asScopedLock();
    if (!lock.lock(50 / portTICK_PERIOD_MS)) {
        return LoaderStatus::ErrorInternal;
    }

    auto previous_app = !loader_singleton->appStack.empty() ? loader_singleton->appStack.top() : nullptr;

    auto new_app = std::make_shared<app::AppInstance>(manifest, parameters);

    new_app->mutableFlags().showStatusbar = (manifest->type != app::Type::Boot);

    loader_singleton->appStack.push(new_app);
    transitionAppToState(new_app, app::State::Initial);
    transitionAppToState(new_app, app::State::Started);

    // We might have to hide the previous app first
    if (previous_app != nullptr) {
        transitionAppToState(previous_app, app::State::Hiding);
    }

    transitionAppToState(new_app, app::State::Showing);

    LoaderEvent event_external = { .type = LoaderEventTypeApplicationStarted };
    loader_singleton->pubsubExternal->publish(&event_external);

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

    auto manifest = app::findAppById(id);
    if (manifest == nullptr) {
        TT_LOG_E(TAG, "App not found: %s", id.c_str());
        return LoaderStatus::ErrorUnknownApp;
    } else {
        return startAppWithManifestInternal(manifest, parameters);
    }
}

static void stopAppInternal() {
    tt_check(loader_singleton != nullptr);

    auto lock = loader_singleton->mutex.asScopedLock();
    if (!lock.lock(50 / portTICK_PERIOD_MS)) {
        return;
    }

    size_t original_stack_size = loader_singleton->appStack.size();

    if (original_stack_size == 0) {
        TT_LOG_E(TAG, "Stop app: no app running");
        return;
    }

    // Stop current app
    auto app_to_stop = loader_singleton->appStack.top();

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

    loader_singleton->appStack.pop();

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
    if (!loader_singleton->appStack.empty()) {
        instance_to_resume = loader_singleton->appStack.top();
        assert(instance_to_resume);
        transitionAppToState(instance_to_resume, app::State::Showing);
    }

    // Unlock so that we can send results to app and they can also start/stop new apps while processing these results
    lock.unlock();
    // WARNING: After this point we cannot change the app states from this method directly anymore as we don't have a lock!

    LoaderEvent event_external = { .type = LoaderEventTypeApplicationStopped };
    loader_singleton->pubsubExternal->publish(&event_external);

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

// region AppManifest

class LoaderService final : public Service {

public:

    void onStart(TT_UNUSED ServiceContext& service) final {
        tt_check(loader_singleton == nullptr);
        loader_singleton = loader_alloc();
        loader_singleton->dispatcherThread->start();
    }

    void onStop(TT_UNUSED ServiceContext& service) final {
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
};

extern const ServiceManifest manifest = {
    .id = "Loader",
    .createService = create<LoaderService>
};

// endregion

} // namespace
