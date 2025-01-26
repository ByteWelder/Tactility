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


/**
 * A thread-safe way to defer code execution.
 * Generally, one task would dispatch the execution,
 * while the other thread consumes and executes the work.
 */
class Dispatcher {
public:

    typedef void (*Function)(std::shared_ptr<void> data);

private:
    struct DispatcherMessage {
        Function function;
        std::shared_ptr<void> context; // Can't use unique_ptr with void, so we use shared_ptr

        DispatcherMessage(Function function, std::shared_ptr<void> context) :
            function(function),
            context(std::move(context))
        {}

        ~DispatcherMessage() = default;
    };

    Mutex mutex;
    std::queue<std::shared_ptr<DispatcherMessage>> queue;
    EventFlag eventFlag;

public:

    explicit Dispatcher() = default;
    ~Dispatcher();

    /**
     * Queue a function to be consumed elsewhere.
     * @param[in] function the function to execute elsewhere
     * @param[in] context the data to pass onto the function
     */
    void dispatch(Function function, std::shared_ptr<void> context);

    /**
     * Consume 1 or more dispatched function (if any) until the queue is empty.
     * @warning The timeout is only the wait time before consuming the message! It is not a limit to the total execution time when calling this method.
     * @param[in] timeout the ticks to wait for a message
     * @return the amount of messages that were consumed
     */
    uint32_t consume(TickType_t timeout = portMAX_DELAY);
};

} // namespace
