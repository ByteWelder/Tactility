#pragma once
#include "api_lock.h"
#include "app_manifest.h"
#include "loader.h"
#include "message_queue.h"
#include "pubsub.h"
#include "thread.h"

typedef struct {
    char* args;
    FuriThread* thread;
    App* app;
} LoaderAppData;

struct Loader {
    FuriPubSub* pubsub;
    FuriMessageQueue* queue;
    LoaderAppData app_data;
};

typedef enum {
    LoaderMessageTypeStartByName,
    LoaderMessageTypeAppClosed,
    LoaderMessageTypeLock,
    LoaderMessageTypeUnlock,
    LoaderMessageTypeIsLocked,
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
