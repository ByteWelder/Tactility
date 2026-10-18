#pragma once

#include <tactility/concurrent/mutex.h>
#include <tactility/concurrent/thread.h>

#include <functional>

namespace tt {

/**
 * Runs functions in order on a task that is pinned to the main dispatcher's CPU core.
 * The task is created when a function is queued and no task is running, and it ends when the queue is empty.
 * Its stack is in internal RAM, so the functions can access flash.
 */
class [[deprecated("Use an app task's stack or a custom task with its own stack")]] MainDispatcher final {

public:

    using Function = std::function<void()>;

private:

    struct Node;

    ::Mutex mutex;
    Node* head = nullptr;
    Node* tail = nullptr;
    bool running = false;
    // The task that has ended or is ending, as it can't free itself
    ::Thread* stale = nullptr;

    static int32_t taskMain(void* context);
    Node* popOrStop();
    /** Must be called with the mutex held and no task running */
    bool startTask();

public:

    MainDispatcher();
    ~MainDispatcher() { mutex_destruct(&mutex); }
    MainDispatcher(const MainDispatcher&) = delete;
    MainDispatcher& operator=(const MainDispatcher&) = delete;

    /**
     * Queue a function to be run on the main dispatcher's task.
     * @param[in] function the function to execute elsewhere
     * @return true if the function was queued
     */
    bool dispatch(Function function);
};

/** Provides access to the dispatcher that runs on the main dispatcher's task.
 * @warning This dispatcher is used for WiFi and might block for some time during WiFi connection.
 * @return the one dispatcher instance
 */
[[deprecated("Use an app task's stack or a custom task with its own stack")]]
MainDispatcher& getMainDispatcher();

} // namespace tt
