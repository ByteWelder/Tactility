/**
* @file Dispatcher.h
*
* Dispatcher is a thread-safe code execution queue.
*/
#pragma once

#include "MessageQueue.h"
#include "Mutex.h"

namespace tt {

typedef void (*Callback)(void* data);

class Dispatcher {
private:
    typedef struct {
        Callback callback;
        void* context;
    } DispatcherMessage;

    MessageQueue queue;
    Mutex mutex;
    DispatcherMessage buffer; // Buffer for consuming a message

public:

    explicit Dispatcher(size_t queueLimit = 8);
    ~Dispatcher();

    void dispatch(Callback callback, void* context);
    bool consume(uint32_t timeout_ticks);
};

} // namespace
