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

    typedef enum {
        StateStopped,
        StateStarting,
        StateRunning,
    } State;

    /** ThreadPriority */
    typedef enum {
        PriorityNone = 0, /**< Uninitialized, choose system default */
        PriorityIdle = 1,
        PriorityLowest = 2,
        PriorityLow = 3,
        PriorityNormal = 4,
        PriorityHigh = 5,
        PriorityHigher = 6,
        PriorityHighest = 7
    } Priority;


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

        // Keep all non-alignable byte types in one place,
        // this ensures that the size of this structure is minimal
        bool isStatic;

        configSTACK_DEPTH_TYPE stackSize;
    } Data;

    Thread();

    /** Allocate Thread, shortcut version
     * @param[in] name
     * @param[in] stack_size
     * @param[in] callback
     * @param[in] callbackContext
     * @return Thread*
     */
    Thread(
        const std::string& name,
        configSTACK_DEPTH_TYPE stackSize,
        Callback callback,
        _Nullable void* callbackContext
    );

    ~Thread();

    /** Set Thread name
     * @param[in] name string
     */
    void setName(const std::string& name);

    /** Mark thread as service
     * The service cannot be stopped or removed, and cannot exit from the thread body
     */
    void markAsStatic();

    /** Check if thread is as service
     * If true, the service cannot be stopped or removed, and cannot exit from the thread body
     */
    bool isMarkedAsStatic() const;

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

    /** Start Thread
     */
    void start();

    /** Join Thread
     * @warning Use this method only when CPU is not busy (Idle task receives control), otherwise it will wait forever.
     * @return success result
     */
    bool join();

    /** Get FreeRTOS ThreadId for Thread instance
     * @return ThreadId or nullptr
     */
    ThreadId getId();

    /** @return thread return code */
    int32_t getReturnCode();

private:

    Data data;
};

#define THREAD_PRIORITY_APP Thread::PriorityNormal
#define THREAD_PRIORITY_SERVICE Thread::PriorityHigh
#define THREAD_PRIORITY_RENDER Thread::PriorityHigher
#define THREAD_PRIORITY_ISR (TT_CONFIG_THREAD_MAX_PRIORITIES - 1)

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

/** Check if the thread was created with static memory
 * @param[in] threadId thread id
 * @return true if thread memory is static
 */
bool thread_mark_is_static(ThreadId threadId);

} // namespace
