#pragma once

#include "api_lock.h"
#include "app_manifest.h"
#include "loader.h"
#include "message_queue.h"
#include "pubsub.h"
#include "services/gui/view_port.h"
#include "thread.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#define APP_STACK_SIZE 32

struct Loader {
    Thread* thread;
    PubSub* pubsub_internal;
    PubSub* pubsub_external;
    MessageQueue* queue;
    Mutex* mutex;
    int8_t app_stack_index;
    App app_stack[APP_STACK_SIZE];
};

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
    ApiLock api_lock;
    LoaderMessageType type;

    struct {
        union {
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

    void setLock(ApiLock lock) {
        api_lock = lock;
    }

    void cleanup() {
        if (type == LoaderMessageTypeAppStart) {
            delete payload.start;
        }
    }
};
