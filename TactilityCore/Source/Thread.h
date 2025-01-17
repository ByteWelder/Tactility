#pragma once

#include "CoreDefines.h"
#include "CoreTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "RtosCompatTask.h"

namespace tt {

typedef TaskHandle_t ThreadId;

class Thread {
public:

    enum class State{
        Stopped,
        Starting,
        Running,
    };

    /** ThreadPriority */
    enum class Priority : UBaseType_t {
        None = 0U, /**< Uninitialized, choose system default */
        Idle = 1U,
        Lower = 2U,
        Low = 3U,
        Normal = 4U,
        High = 5U,
        Higher = 6U,
        Critical = 7U
    };

    /** ThreadCallback Your callback to run in new thread
     * @warning never use osThreadExit in Thread
     */
    typedef int32_t (*Callback)(void* context);

    /** Write to stdout callback
     * @param[in] data pointer to data
     * @param[in] size data size @warning your handler must consume everything
     */
    typedef void (*StdoutWriteCallback)(const char* data, size_t size);

    /** Thread state change callback called upon thread state change
     * @param[in] state new thread state
     * @param[in] context callback context
     */
    typedef void (*StateCallback)(State state, void* context);

    typedef struct {
        Thread* thread;
        TaskHandle_t taskHandle;
        State state;
        Callback callback;
        void* callbackContext;
        int32_t callbackResult;
        StateCallback stateCallback;
        void* stateCallbackContext;
        std::string name;
        Priority priority;
        configSTACK_DEPTH_TYPE stackSize;
        portBASE_TYPE affinity;
    } Data;

    Thread();

    /** Allocate Thread, shortcut version
     * @param[in] name the name of the thread
     * @param[in] stackSize in bytes
     * @param[in] callback
     * @param[in] callbackContext
     * @param[in] affinity Which CPU core to pin this task to, -1 means unpinned (only works on ESP32)
     */
    Thread(
        const std::string& name,
        configSTACK_DEPTH_TYPE stackSize,
        Callback callback,
        _Nullable void* callbackContext,
        portBASE_TYPE affinity = -1
    );

    ~Thread();

    /** Set Thread name
     * @param[in] name string
     */
    void setName(const std::string& name);

    /** Set Thread stack size
     * @param[in] stackSize stack size in bytes
     */
    void setStackSize(size_t stackSize);

    /** Set Thread callback
     * @param[in] callback ThreadCallback, called upon thread run
     * @param[in] callbackContext what to pass to the callback
     */
    void setCallback(Callback callback, _Nullable void* callbackContext = nullptr);

    /** Set Thread priority
     * @param[in] priority ThreadPriority value
     */
    void setPriority(Priority priority);


    /** Set Thread state change callback
     * @param[in] callback state change callback
     * @param[in] callbackContext pointer to context
     */
    void setStateCallback(StateCallback callback, _Nullable void* callbackContext = nullptr);

    /** Get Thread state
     * @return thread state from ThreadState
     */
    State getState() const;

    /** Start Thread */
    void start();

    /** Join Thread
     * @warning make sure you manually interrupt any logic in your thread (e.g. by an EventFlag or boolean+Mutex)
     * @param[in] timeout the maximum amount of time to wait
     * @param[in] pollInterval the amount of ticks to wait before we check again if the thread is finished
     * @return success result
     */
    bool join(TickType_t timeout = portMAX_DELAY, TickType_t pollInterval = 10);

    /** Get FreeRTOS ThreadId for Thread instance
     * @return ThreadId or nullptr
     */
    ThreadId getId() const;

    /**
     * @warning crashes when state is not "stopped"
     * @return thread return code
     */
    int32_t getReturnCode() const;

private:

    Data data;
};

#define THREAD_PRIORITY_APP Thread::PriorityNormal
#define THREAD_PRIORITY_SERVICE Thread::Priority::High
#define THREAD_PRIORITY_RENDER Thread::Priority::Higher
#define THREAD_PRIORITY_ISR Thread::Priority::Critical

/** Set current thread priority
 * @param[in] priority ThreadPriority value
 */
void thread_set_current_priority(Thread::Priority priority);

/** @return ThreadPriority value */
Thread::Priority thread_get_current_priority();

/** @return FreeRTOS ThreadId or NULL */
ThreadId thread_get_current_id();

/** @return pointer to Thread instance or NULL if this thread doesn't belongs to Tactility */
Thread* thread_get_current();

/** Return control to scheduler */
void thread_yield();

uint32_t thread_flags_set(ThreadId thread_id, uint32_t flags);

uint32_t thread_flags_clear(uint32_t flags);

uint32_t thread_flags_get();

uint32_t thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout);

/**
 * @brief Get thread name
 * @param[in] threadId
 * @return const char* name or NULL
 */
const char* thread_get_name(ThreadId threadId);

/**
 * @brief Get thread stack watermark
 * @param[in] threadId
 * @return uint32_t 
 */
uint32_t thread_get_stack_space(ThreadId threadId);

/** Suspend thread
 * @param[in] threadId thread id
 */
void thread_suspend(ThreadId threadId);

/** Resume thread
 * @param[in] threadId thread id
 */
void thread_resume(ThreadId threadId);

/** Get thread suspended state
 * @param[in] threadId thread id
 * @return true if thread is suspended
 */
bool thread_is_suspended(ThreadId threadId);

} // namespace
