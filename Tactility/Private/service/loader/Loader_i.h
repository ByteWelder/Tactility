#pragma once

#include "app/AppManifest.h"
#include "app/AppInstance.h"
#include "EventFlag.h"
#include "MessageQueue.h"
#include "Pubsub.h"
#include "Thread.h"
#include "service/gui/ViewPort.h"
#include "service/loader/Loader.h"
#include "RtosCompatSemaphore.h"
#include <stack>
#include <utility>
#include <DispatcherThread.h>

namespace tt::service::loader {


// region LoaderEvent

typedef enum {
    LoaderEventTypeApplicationStarted,
    LoaderEventTypeApplicationShowing,
    LoaderEventTypeApplicationHiding,
    LoaderEventTypeApplicationStopped
} LoaderEventType;

typedef struct {
    app::AppInstance& app;
} LoaderEventAppStarted;

typedef struct {
    app::AppInstance& app;
} LoaderEventAppShowing;

typedef struct {
    app::AppInstance& app;
} LoaderEventAppHiding;

typedef struct {
    const app::AppManifest& manifest;
} LoaderEventAppStopped;

typedef struct {
    LoaderEventType type;
    union {
        LoaderEventAppStarted app_started;
        LoaderEventAppShowing app_showing;
        LoaderEventAppHiding app_hiding;
        LoaderEventAppStopped app_stopped;
    };
} LoaderEvent;

// endregion LoaderEvent

// region LoaderMessage

class LoaderMessageAppStart {
public:
    // This lock blocks anyone from starting an app as long
    // as an app is already running via loader_start()
    // This lock's lifecycle is not owned by this class.
    std::shared_ptr<EventFlag> api_lock = std::make_shared<EventFlag>();
    std::string id;
    std::shared_ptr<const Bundle> _Nullable parameters;

    LoaderMessageAppStart() = default;

    LoaderMessageAppStart(LoaderMessageAppStart& other) :
        api_lock(other.api_lock),
        id(other.id),
        parameters(other.parameters) {}

    LoaderMessageAppStart(const std::string& id, std::shared_ptr<const Bundle> parameters) :
        id(id),
        parameters(std::move(parameters))
    {}

    ~LoaderMessageAppStart() = default;

    std::shared_ptr<EventFlag> getApiLockEventFlag() { return api_lock; }

    uint32_t getApiLockEventFlagValue() { return 1; }

    void onProcessed() {
        api_lock->set(1);
    }
};

// endregion LoaderMessage

struct Loader {
    std::shared_ptr<PubSub> pubsub_internal = std::make_shared<PubSub>();
    std::shared_ptr<PubSub> pubsub_external = std::make_shared<PubSub>();
    Mutex mutex = Mutex(Mutex::TypeRecursive);
    std::stack<app::AppInstance*> app_stack;
    std::unique_ptr<DispatcherThread> dispatcherThread = std::make_unique<DispatcherThread>("loader_dispatcher", 6144); // Files app requires ~5k
};

} // namespace
