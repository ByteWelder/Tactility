#pragma once
#include "api_lock.h"
#include "app_manifest.h"
#include "loader.h"
#include "message_queue.h"
#include "pubsub.h"
#include "thread.h"
#include "apps/services/gui/view_port.h"

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
};

typedef enum {
    LoaderMessageTypeStartByName,
    LoaderMessageTypeAppStop,
    LoaderMessageTypeExit,
} LoaderMessageType;

typedef struct {
    const char* id;
    const char* args;
    FuriString* error_message;
} LoaderMessageStartById;

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
        LoaderMessageStartById start;
    };

    union {
        LoaderMessageLoaderStatusResult* status_value;
        LoaderMessageBoolResult* bool_value;
    };
} LoaderMessage;
