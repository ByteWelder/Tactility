#pragma once

#include "Dispatcher.h"

namespace tt {

/** Starts a Thread to process dispatched messages */
class DispatcherThread final {

    Dispatcher dispatcher;
    std::unique_ptr<Thread> thread;
    bool interruptThread = true;

    int32_t threadMain();

public:

    explicit DispatcherThread(const std::string& threadName, size_t threadStackSize = 4096);
    ~DispatcherThread();

    /**
     * Dispatch a message.
     */
    bool dispatch(Dispatcher::Function function, TickType_t timeout = portMAX_DELAY);

    /** Start the thread (blocking). */
    void start();

    /** Stop the thread (blocking). */
    void stop();

    /** @return true of the thread is started */
    bool isStarted() const { return thread != nullptr && !interruptThread; }
};

}