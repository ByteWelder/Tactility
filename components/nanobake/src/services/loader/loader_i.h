#pragma once

#include "api_lock.h"
#include "app_manifest.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "loader.h"
#include "message_queue.h"
#include "pubsub.h"
#include "services/gui/view_port.h"
#include "thread.h"

typedef struct {
    char* args;
    App* app;
    ViewPort* view_port;
} LoaderAppData;

struct Loader {
    FuriThread* thread;
    FuriPubSub* pubsub;
    FuriMessageQueue* queue;
    LoaderAppData app_data;
    SemaphoreHandle_t mutex;
};

typedef enum {
    LoaderMessageTypeAppStart,
    LoaderMessageTypeAppStop,
    LoaderMessageTypeServiceStop,
} LoaderMessageType;

typedef struct {
    const char* id;
    const char* args;
    FuriString* error_message;
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
    FuriApiLock api_lock;
    LoaderMessageType type;

    union {
        LoaderMessageAppStart start;
    };

    union {
        LoaderMessageLoaderStatusResult* status_value;
        LoaderMessageBoolResult* bool_value;
    };
} LoaderMessage;
