/**
* @file Dispatcher.h
*
* Dispatcher is a thread-safe code execution queue.
*/
#pragma once

#include "MessageQueue.h"
#include "Mutex.h"
#include "EventFlag.h"
#include <memory>
#include <queue>

namespace tt {

typedef void (*Callback)(std::shared_ptr<void> data);

class Dispatcher {
private:
    struct DispatcherMessage {
        Callback callback;
        std::shared_ptr<void> context; // Can't use unique_ptr with void, so we use shared_ptr

        DispatcherMessage(Callback callback, std::shared_ptr<void> context) :
            callback(callback),
            context(std::move(context))
        {}

        ~DispatcherMessage() = default;
    };

    Mutex mutex;
    std::queue<std::shared_ptr<DispatcherMessage>> queue;
    EventFlag eventFlag;

public:

    explicit Dispatcher();
    ~Dispatcher();

    void dispatch(Callback callback, std::shared_ptr<void> context);
    uint32_t consume(uint32_t timeout_ticks);
};

} // namespace
