#pragma once

#include "Dispatcher.h"

namespace tt {

/** Starts a Thread to process dispatched messages */
class DispatcherThread {

    Dispatcher dispatcher;
    std::unique_ptr<Thread> thread;
    bool interruptThread = false;

public:

    explicit DispatcherThread(const std::string& threadName, size_t threadStackSize = 9000);
    ~DispatcherThread();

    /**
     * Dispatch a message.
     */
    void dispatch(Callback callback, std::shared_ptr<void> context);

    /** Start the thread (blocking). */
    void start();

    /** Stop the thread (blocking). */
    void stop();

    /** Internal method */
    void _threadMain();
};

}