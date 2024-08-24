/**
* @file message_queue.h
*
* Dispatcher is a thread-safe message queue implementation for callbacks.
*/
#pragma once

#include "message_queue.h"
#include "mutex.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*Callback)(void* data);

typedef struct {
    Callback callback;
    void* context;
} DispatcherMessage;

typedef struct {
    MessageQueue* queue;
    Mutex* mutex;
    DispatcherMessage buffer; // Buffer for consuming a message
} Dispatcher;

Dispatcher* tt_dispatcher_alloc(uint32_t message_count);
void tt_dispatcher_free(Dispatcher* dispatcher);
void tt_dispatcher_dispatch(Dispatcher* dispatcher, Callback callback, void* context);
bool tt_dispatcher_consume(Dispatcher* dispatcher, uint32_t timeout_ticks);

#ifdef __cplusplus
}
#endif