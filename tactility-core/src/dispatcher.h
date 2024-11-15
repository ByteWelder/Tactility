/**
* @file dispatcher.h
*
* Dispatcher is a thread-safe code execution queue.
*/
#pragma once

#include "message_queue.h"
#include "mutex.h"

typedef void (*Callback)(void* data);

typedef void Dispatcher;

Dispatcher* tt_dispatcher_alloc(uint32_t message_count);
void tt_dispatcher_free(Dispatcher* dispatcher);
void tt_dispatcher_dispatch(Dispatcher* data, Callback callback, void* context);
bool tt_dispatcher_consume(Dispatcher* data, uint32_t timeout_ticks);