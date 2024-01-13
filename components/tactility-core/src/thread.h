/**
 * @file thread.h
 * Furi: Furi Thread API
 */

#pragma once

#include "core_defines.h"
#include "core_types.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ThreadState */
typedef enum {
    ThreadStateStopped,
    ThreadStateStarting,
    ThreadStateRunning,
} ThreadState;

/** ThreadPriority */
typedef enum {
    ThreadPriorityNone = 0,     /**< Uninitialized, choose system default */
    ThreadPriorityIdle = 1,     /**< Idle priority */
    ThreadPriorityLowest = 14,  /**< Lowest */
    ThreadPriorityLow = 15,     /**< Low */
    ThreadPriorityNormal = 16,  /**< Normal */
    ThreadPriorityHigh = 17,    /**< High */
    ThreadPriorityHighest = 18, /**< Highest */
    ThreadPriorityIsr =
        (TT_CONFIG_THREAD_MAX_PRIORITIES - 1), /**< Deferred ISR (highest possible) */
} ThreadPriority;

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
Thread* tt_thread_alloc();

/** Allocate Thread, shortcut version
 * 
 * @param name 
 * @param stack_size 
 * @param callback 
 * @param context 
 * @return Thread*
 */
Thread* tt_thread_alloc_ex(
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
void tt_thread_free(Thread* thread);

/** Set Thread name
 *
 * @param      thread  Thread instance
 * @param      name    string
 */
void tt_thread_set_name(Thread* thread, const char* name);

/**
 * @brief Set Thread appid
 * Technically, it is like a "process id", but it is not a system-wide unique identifier.
 * All threads spawned by the same app will have the same appid.
 * 
 * @param thread 
 * @param appid 
 */
void tt_thread_set_appid(Thread* thread, const char* appid);

/** Mark thread as service
 * The service cannot be stopped or removed, and cannot exit from the thread body
 * 
 * @param thread 
 */
void tt_thread_mark_as_static(Thread* thread);

/** Set Thread stack size
 *
 * @param      thread      Thread instance
 * @param      stack_size  stack size in bytes
 */
void tt_thread_set_stack_size(Thread* thread, size_t stack_size);

/** Set Thread callback
 *
 * @param      thread    Thread instance
 * @param      callback  ThreadCallback, called upon thread run
 */
void tt_thread_set_callback(Thread* thread, ThreadCallback callback);

/** Set Thread context
 *
 * @param      thread   Thread instance
 * @param      context  pointer to context for thread callback
 */
void tt_thread_set_context(Thread* thread, void* context);

/** Set Thread priority
 *
 * @param      thread   Thread instance
 * @param      priority ThreadPriority value
 */
void tt_thread_set_priority(Thread* thread, ThreadPriority priority);

/** Set current thread priority
 *
 * @param      priority ThreadPriority value
 */
void tt_thread_set_current_priority(ThreadPriority priority);

/** Get current thread priority
 *
 * @return     ThreadPriority value
 */
ThreadPriority tt_thread_get_current_priority();

/** Set Thread state change callback
 *
 * @param      thread    Thread instance
 * @param      callback  state change callback
 */
void tt_thread_set_state_callback(Thread* thread, ThreadStateCallback callback);

/** Set Thread state change context
 *
 * @param      thread   Thread instance
 * @param      context  pointer to context
 */
void tt_thread_set_state_context(Thread* thread, void* context);

/** Get Thread state
 *
 * @param      thread  Thread instance
 *
 * @return     thread state from ThreadState
 */
ThreadState tt_thread_get_state(Thread* thread);

/** Start Thread
 *
 * @param      thread  Thread instance
 */
void tt_thread_start(Thread* thread);

/** Join Thread
 *
 * @warning    Use this method only when CPU is not busy(Idle task receives
 *             control), otherwise it will wait forever.
 *
 * @param      thread  Thread instance
 *
 * @return     bool
 */
bool tt_thread_join(Thread* thread);

/** Get FreeRTOS ThreadId for Thread instance
 *
 * @param      thread  Thread instance
 *
 * @return     ThreadId or NULL
 */
ThreadId tt_thread_get_id(Thread* thread);

/** Enable heap tracing
 *
 * @param      thread  Thread instance
 */
void tt_thread_enable_heap_trace(Thread* thread);

/** Disable heap tracing
 *
 * @param      thread  Thread instance
 */
void tt_thread_disable_heap_trace(Thread* thread);

/** Get thread heap size
 *
 * @param      thread  Thread instance
 *
 * @return     size in bytes
 */
size_t tt_thread_get_heap_size(Thread* thread);

/** Get thread return code
 *
 * @param      thread  Thread instance
 *
 * @return     return code
 */
int32_t tt_thread_get_return_code(Thread* thread);

/** Thread related methods that doesn't involve Thread directly */

/** Get FreeRTOS ThreadId for current thread
 *
 * @param      thread  Thread instance
 *
 * @return     ThreadId or NULL
 */
ThreadId tt_thread_get_current_id();

/** Get Thread instance for current thread
 * 
 * @return pointer to Thread or NULL if this thread doesn't belongs to Furi
 */
Thread* tt_thread_get_current();

/** Return control to scheduler */
void tt_thread_yield();

uint32_t tt_thread_flags_set(ThreadId thread_id, uint32_t flags);

uint32_t tt_thread_flags_clear(uint32_t flags);

uint32_t tt_thread_flags_get(void);

uint32_t tt_thread_flags_wait(uint32_t flags, uint32_t options, uint32_t timeout);

/**
 * @brief Enumerate threads
 * 
 * @param thread_array array of ThreadId, where thread ids will be stored
 * @param array_items array size
 * @return uint32_t threads count
 */
uint32_t tt_thread_enumerate(ThreadId* thread_array, uint32_t array_items);

/**
 * @brief Get thread name
 * 
 * @param thread_id 
 * @return const char* name or NULL
 */
const char* tt_thread_get_name(ThreadId thread_id);

/**
 * @brief Get thread appid
 * 
 * @param thread_id 
 * @return const char* appid
 */
const char* tt_thread_get_appid(ThreadId thread_id);

/**
 * @brief Get thread stack watermark
 * 
 * @param thread_id 
 * @return uint32_t 
 */
uint32_t tt_thread_get_stack_space(ThreadId thread_id);

/** Get STDOUT callback for thead
 *
 * @return STDOUT callback
 */
ThreadStdoutWriteCallback tt_thread_get_stdout_callback();

/** Set STDOUT callback for thread
 *
 * @param      callback  callback or NULL to clear
 */
void tt_thread_set_stdout_callback(ThreadStdoutWriteCallback callback);

/** Write data to buffered STDOUT
 * 
 * @param data input data
 * @param size input data size
 * 
 * @return size_t written data size
 */
size_t tt_thread_stdout_write(const char* data, size_t size);

/** Flush data to STDOUT
 * 
 * @return int32_t error code
 */
int32_t tt_thread_stdout_flush();

/** Suspend thread
 * 
 * @param thread_id thread id
 */
void tt_thread_suspend(ThreadId thread_id);

/** Resume thread
 * 
 * @param thread_id thread id
 */
void tt_thread_resume(ThreadId thread_id);

/** Get thread suspended state
 * 
 * @param thread_id thread id
 * @return true if thread is suspended
 */
bool tt_thread_is_suspended(ThreadId thread_id);

bool tt_thread_mark_is_service(ThreadId thread_id);

#ifdef __cplusplus
}
#endif
