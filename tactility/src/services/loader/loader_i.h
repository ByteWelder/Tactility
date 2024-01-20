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
    PubSub* pubsub;
    MessageQueue* queue;
    // TODO: replace with Mutex
    SemaphoreHandle_t mutex;
    int8_t app_stack_index;
    App app_stack[APP_STACK_SIZE];
};

typedef enum {
    LoaderMessageTypeAppStart,
    LoaderMessageTypeAppStop,
    LoaderMessageTypeServiceStop,
} LoaderMessageType;

typedef struct {
    const char* id;
    Bundle* _Nullable bundle;
} LoaderMessageAppStart;

typedef struct {
    LoaderStatus value;
} LoaderMessageLoaderStatusResult;

typedef struct {
    bool value;
} LoaderMessageBoolResult;

typedef struct {
    // This lock blocks anyone from starting an app as long
    // as an app is already running via loader_start()
    ApiLock api_lock;
    LoaderMessageType type;

    union {
        LoaderMessageAppStart start;
    };

    union {
        LoaderMessageLoaderStatusResult* status_value;
        LoaderMessageBoolResult* bool_value;
    };
} LoaderMessage;
