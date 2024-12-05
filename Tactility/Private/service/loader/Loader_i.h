#pragma once

#include "app/AppManifest.h"
#include "app/AppInstance.h"
#include "MessageQueue.h"
#include "Pubsub.h"
#include "Thread.h"
#include "service/gui/ViewPort.h"
#include "service/loader/Loader.h"
#include "RtosCompatSemaphore.h"
#include <stack>

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

typedef enum {
    LoaderMessageTypeNone,
    LoaderMessageTypeAppStart,
    LoaderMessageTypeAppStop,
    LoaderMessageTypeServiceStop,
} LoaderMessageType;

class LoaderMessageAppStart {
public:
    std::string id;
    Bundle bundle;

    LoaderMessageAppStart() = default;

    LoaderMessageAppStart(const std::string& id, const Bundle& bundle) {
        this->id = id;
        this->bundle = bundle;
    }
};

typedef struct {
    LoaderStatus value;
} LoaderMessageLoaderStatusResult;

typedef struct {
    bool value;
} LoaderMessageBoolResult;

class LoaderMessage {
public:
    // This lock blocks anyone from starting an app as long
    // as an app is already running via loader_start()
    // This lock's lifecycle is not owned by this class.
    EventFlag* _Nullable api_lock;
    LoaderMessageType type;

    struct {
        union {
            // TODO: Convert to smart pointer
            const LoaderMessageAppStart* start;
        };
    } payload;

    struct {
        union {
            LoaderMessageLoaderStatusResult status_value;
            LoaderMessageBoolResult bool_value;
            void* raw_value;
        };
    } result;

    LoaderMessage() {
        api_lock = nullptr;
        type = LoaderMessageTypeNone;
        payload = { .start = nullptr };
        result = { .raw_value = nullptr };
    }

    LoaderMessage(const LoaderMessageAppStart* start, const LoaderMessageLoaderStatusResult& statusResult) {
        api_lock = nullptr;
        type = LoaderMessageTypeAppStart;
        payload.start = start;
        result.status_value = statusResult;
    }

    LoaderMessage(LoaderMessageType messageType) {
        api_lock = nullptr;
        type = messageType;
        payload = { .start = nullptr };
        result = { .raw_value = nullptr };
    }

    void setApiLock(EventFlag* eventFlag) {
        api_lock = eventFlag;
    }

    void cleanup() {
        if (type == LoaderMessageTypeAppStart) {
            delete payload.start;
        }
    }
};

// endregion LoaderMessage

struct Loader {
    Thread* thread;
    PubSub* pubsub_internal;
    PubSub* pubsub_external;
    MessageQueue queue = MessageQueue(2, sizeof(LoaderMessage)); // 2 entries, so you can stop the current app while starting a new one without blocking
    Mutex* mutex;
    std::stack<app::AppInstance*> app_stack;
};

} // namespace
