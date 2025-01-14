#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** The handle that represents the thread insance */
typedef void* ThreadHandle;

/** The state of a thread instance */
typedef enum {
    ThreadStateStopped,
    ThreadStateStarting,
    ThreadStateRunning,
} ThreadState;

/** The identifier that represents the thread */
typedef TaskHandle_t ThreadId;

/** ThreadCallback Your callback to run in new thread
 * @warning never use osThreadExit in Thread
 */
typedef int32_t (*ThreadCallback)(void* context);

/** Thread state change callback called upon thread state change
 * @param state new thread state
 * @param context callback context
 */
typedef void (*ThreadStateCallback)(ThreadState state, void* context);

typedef enum {
    ThreadPriorityNone = 0U, /**< Uninitialized, choose system default */
    ThreadPriorityIdle = 1U,
    ThreadPriorityLowest = 2U,
    ThreadPriorityLow = 3U,
    ThreadPriorityNormal = 4U,
    ThreadPriorityHigh = 5U,
    ThreadPriorityHigher = 6U,
    ThreadPriorityHighest = 7U
} ThreadPriority;

/** @return a thread handle that represents a newly allocated thread instance */
ThreadHandle tt_thread_alloc();

/**
 * Allocate a thread and provide some common parameters so it's all ready to be started.
 * @param[in] name the name of the thread
 * @param[in] stackSize the size of the stack in bytes
 * @param[in] callback the callback to call from the thread
 * @param[in] callbackContext the data to pass to the callback
 */
ThreadHandle tt_thread_alloc_ext(
    const char* name,
    uint32_t stackSize,
    ThreadCallback callback,
    void* _Nullable callbackContext
);

/**
 * Free up the memory of the thread that is represented by this handle
 * @param[in] handle the thread instance handle
 */
void tt_thread_free(ThreadHandle handle);

/**
 * Set the name of a thread
 * @param[in] handle the thread instance handle
 * @param[in] name the name to set
 */
void tt_thread_set_name(ThreadHandle handle, const char* name);

/**
 * Set the stack size of the thread (in bytes)
 * @param[in] handle the thread instance handle
 * @param[in] the size of the thread in bytes
 */
void tt_thread_set_stack_size(ThreadHandle handle, size_t size);

/**
 * Set the callback for a thread. This method is executed when the thread is started.
 * @param[in] handle the thread instance handle
 * @param[in] callback the callback to set
 * @param[in] callbackContext the data to pass to the callback
 */
void tt_thread_set_callback(ThreadHandle handle, ThreadCallback callback, void* _Nullable callbackContext);

/**
 * Set the priority of a thread
 * @param[in] handle the thread instance handle
 * @param[in] priority the priority to set
 */
void tt_thread_set_priority(ThreadHandle handle, ThreadPriority priority);

/**
 * Set the state callback for a thread
 * @param[in] handle the thread instance handle
 * @param[in] callback the callback to set
 * @param[in] callbackContext the data to pass to the callback
 */
void tt_thread_set_state_callback(ThreadHandle handle, ThreadStateCallback callback, void* _Nullable callbackContext);

/**
 * @param[in] handle the thread instance handle
 * @return the current state of a thread
 */
ThreadState tt_thread_get_state(ThreadHandle handle);

/**
 * Start a thread
 * @param[in] handle the thread instance handle
 */
void tt_thread_start(ThreadHandle handle);

/**
 * Wait (block) for the thread to finish.
 * @param[in] handle the thread instance handle
 * @warning make sure you manually interrupt any logic in your thread (e.g. by an EventFlag or boolean+Mutex)
 */
bool tt_thread_join(ThreadHandle handle, TickType_t timeout);

/**
 * Get thread id
 * @param[in] handle the thread instance handle
 * @return the ThreadId of a thread
 * */
ThreadId tt_thread_get_id(ThreadHandle handle);

/**
 * Get the return code of a thread
 * @warning crashes when state is not "stopped"
 * @param[in] handle the thread instance handle
 * @return the return code of a thread or
 */
int32_t tt_thread_get_return_code(ThreadHandle handle);

#ifdef __cplusplus
}
#endif