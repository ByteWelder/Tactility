#pragma once

#include "CoreDefines.h"
#include "CoreTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#endif

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
     * @param data pointer to data
     * @param size data size @warning your handler must consume everything
     */
    typedef void (*StdoutWriteCallback)(const char* data, size_t size);

    /** Thread state change callback called upon thread state change
     * @param state new thread state
     * @param context callback context
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

     * @param name
     * @param stack_size
     * @param callback
     * @param context
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
     *
     * @param name string
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
     *
     * @param thread Thread instance
     * @param stackSize stack size in bytes
     */
    void setStackSize(size_t stackSize);

    /** Set Thread callback
     *
     * @param thread Thread instance
     * @param callback ThreadCallback, called upon thread run
     * @param callbackContext what to pass to the callback
     */
    void setCallback(Callback callback, _Nullable void* callbackContext = nullptr);

    /** Set Thread priority
     *
     * @param thread Thread instance
     * @param priority ThreadPriority value
     */
    void setPriority(Priority priority);


    /** Set Thread state change callback
     *
     * @param thread Thread instance
     * @param callback state change callback
     * @param context pointer to context
     */
    void setStateCallback(StateCallback callback, _Nullable void* callbackContext = nullptr);

    /** Get Thread state
     *
     * @param thread Thread instance
     *
     * @return thread state from ThreadState
     */
    [[nodiscard]] State getState() const;

    /** Start Thread
     *
     * @param thread Thread instance
     */
    void start();

    /** Join Thread
     *
     * @warning    Use this method only when CPU is not busy(Idle task receives
     *             control), otherwise it will wait forever.
     *
     * @param thread Thread instance
     *
     * @return success result
     */
    bool join();

    /** Get FreeRTOS ThreadId for Thread instance
     *
     * @param thread Thread instance
     *
     * @return ThreadId or nullptr
     */
    ThreadId getId();

    /** Get thread return code
     *
     * @param thread Thread instance
     *
     * @return return code
     */
    int32_t getReturnCode();

private:

    Data data;
};

#define THREAD_PRIORITY_APP Thread::PriorityNormal
#define THREAD_PRIORITY_SERVICE Thread::PriorityHigh
#define THREAD_PRIORITY_RENDER Thread::PriorityHigher
#define THREAD_PRIORITY_ISR (TT_CONFIG_THREAD_MAX_PRIORITIES - 1)

/** Set current thread priority
 *
 * @param      priority ThreadPriority value
 */
void thread_set_current_priority(Thread::Priority priority);

/** Get current thread priority
 *
 * @return     ThreadPriority value
 */
Thread::Priority thread_get_current_priority();

/** Thread related methods that doesn't involve Thread directly */

/** Get FreeRTOS ThreadId for current thread
 *
 * @param      thread  Thread instance
 *
 * @return     ThreadId or NULL
 */
ThreadId thread_get_current_id();

/** Get Thread instance for current thread
 * 
 * @return pointer to Thread or NULL if this thread doesn't belongs to Tactility
 */
Thread* thread_get_current();

/** Return control to scheduler */
void thread_yield();

uint32_t thread_flags_set(ThreadId thread_id, uint32_t flags);

uint32_t thread_flags_clear(uint32_t flags);

uint32_t thread_flags_get();

uint32_t thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout);

/**
 * @brief Get thread name
 * 
 * @param thread_id 
 * @return const char* name or NULL
 */
const char* thread_get_name(ThreadId thread_id);

/**
 * @brief Get thread stack watermark
 * 
 * @param thread_id 
 * @return uint32_t 
 */
uint32_t thread_get_stack_space(ThreadId thread_id);

/** Suspend thread
 * 
 * @param thread_id thread id
 */
void thread_suspend(ThreadId thread_id);

/** Resume thread
 * 
 * @param thread_id thread id
 */
void thread_resume(ThreadId thread_id);

/** Get thread suspended state
 * 
 * @param thread_id thread id
 * @return true if thread is suspended
 */
bool thread_is_suspended(ThreadId thread_id);

/** Check if the thread was created with static memory
 *
 * @param thread_id  thread id
 * @return true if thread memory is static
 */
bool thread_mark_is_static(ThreadId thread_id);

} // namespace
