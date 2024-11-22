#pragma once

#include "CoreDefines.h"
#include "CoreTypes.h"

#include <cstddef>
#include <cstdint>

namespace tt {

/** ThreadState */
typedef enum {
    ThreadStateStopped,
    ThreadStateStarting,
    ThreadStateRunning,
} ThreadState;

/** ThreadPriority */
typedef enum {
    ThreadPriorityNone = 0, /**< Uninitialized, choose system default */
    ThreadPriorityIdle = 1,
    ThreadPriorityLowest = 2,
    ThreadPriorityLow = 3,
    ThreadPriorityNormal = 4,
    ThreadPriorityHigh = 5,
    ThreadPriorityHigher = 6,
    ThreadPriorityHighest = 7
} ThreadPriority;

#define THREAD_PRIORITY_APP ThreadPriorityNormal
#define THREAD_PRIORITY_SERVICE ThreadPriorityHigh
#define THREAD_PRIORITY_RENDER ThreadPriorityHigher
#define THREAD_PRIORITY_ISR (TT_CONFIG_THREAD_MAX_PRIORITIES - 1)

/** Thread anonymous structure */
typedef struct Thread Thread;

/** ThreadId proxy type to OS low level functions */
typedef void* ThreadId;

/** ThreadCallback Your callback to run in new thread
 * @warning    never use osThreadExit in Thread
 */
typedef int32_t (*ThreadCallback)(void* context);

/** Write to stdout callback
 * @param      data     pointer to data
 * @param      size     data size @warning your handler must consume everything
 */
typedef void (*ThreadStdoutWriteCallback)(const char* data, size_t size);

/** Thread state change callback called upon thread state change
 * @param      state    new thread state
 * @param      context  callback context
 */
typedef void (*ThreadStateCallback)(ThreadState state, void* context);

/** Allocate Thread
 *
 * @return     Thread instance
 */
Thread* thread_alloc();

/** Allocate Thread, shortcut version
 * 
 * @param name 
 * @param stack_size 
 * @param callback 
 * @param context 
 * @return Thread*
 */
Thread* thread_alloc_ex(
    const char* name,
    uint32_t stack_size,
    ThreadCallback callback,
    void* context
);

/** Release Thread
 *
 * @warning    see tt_thread_join
 *
 * @param      thread  Thread instance
 */
void thread_free(Thread* thread);

/** Set Thread name
 *
 * @param      thread  Thread instance
 * @param      name    string
 */
void thread_set_name(Thread* thread, const char* name);

/**
 * @brief Set Thread appid
 * Technically, it is like a "process id", but it is not a system-wide unique identifier.
 * All threads spawned by the same app will have the same appid.
 * 
 * @param thread 
 * @param appid 
 */
void thread_set_appid(Thread* thread, const char* appid);

/** Mark thread as service
 * The service cannot be stopped or removed, and cannot exit from the thread body
 * 
 * @param thread 
 */
void thread_mark_as_static(Thread* thread);

/** Set Thread stack size
 *
 * @param      thread      Thread instance
 * @param      stack_size  stack size in bytes
 */
void thread_set_stack_size(Thread* thread, size_t stack_size);

/** Set Thread callback
 *
 * @param      thread    Thread instance
 * @param      callback  ThreadCallback, called upon thread run
 */
void thread_set_callback(Thread* thread, ThreadCallback callback);

/** Set Thread context
 *
 * @param      thread   Thread instance
 * @param      context  pointer to context for thread callback
 */
void thread_set_context(Thread* thread, void* context);

/** Set Thread priority
 *
 * @param      thread   Thread instance
 * @param      priority ThreadPriority value
 */
void thread_set_priority(Thread* thread, ThreadPriority priority);

/** Set current thread priority
 *
 * @param      priority ThreadPriority value
 */
void thread_set_current_priority(ThreadPriority priority);

/** Get current thread priority
 *
 * @return     ThreadPriority value
 */
ThreadPriority thread_get_current_priority();

/** Set Thread state change callback
 *
 * @param      thread    Thread instance
 * @param      callback  state change callback
 */
void thread_set_state_callback(Thread* thread, ThreadStateCallback callback);

/** Set Thread state change context
 *
 * @param      thread   Thread instance
 * @param      context  pointer to context
 */
void thread_set_state_context(Thread* thread, void* context);

/** Get Thread state
 *
 * @param      thread  Thread instance
 *
 * @return     thread state from ThreadState
 */
ThreadState thread_get_state(Thread* thread);

/** Start Thread
 *
 * @param      thread  Thread instance
 */
void thread_start(Thread* thread);

/** Join Thread
 *
 * @warning    Use this method only when CPU is not busy(Idle task receives
 *             control), otherwise it will wait forever.
 *
 * @param      thread  Thread instance
 *
 * @return     bool
 */
bool thread_join(Thread* thread);

/** Get FreeRTOS ThreadId for Thread instance
 *
 * @param      thread  Thread instance
 *
 * @return     ThreadId or NULL
 */
ThreadId thread_get_id(Thread* thread);

/** Get thread return code
 *
 * @param      thread  Thread instance
 *
 * @return     return code
 */
int32_t thread_get_return_code(Thread* thread);

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
 * @brief Get thread appid
 * 
 * @param thread_id 
 * @return const char* appid
 */
const char* thread_get_appid(ThreadId thread_id);

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
