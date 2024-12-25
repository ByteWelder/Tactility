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

typedef void* ThreadHandle;

typedef enum {
    ThreadStateStopped,
    ThreadStateStarting,
    ThreadStateRunning,
} ThreadState;

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
    ThreadPriorityNone = 0, /**< Uninitialized, choose system default */
    ThreadPriorityIdle = 1,
    ThreadPriorityLowest = 2,
    ThreadPriorityLow = 3,
    ThreadPriorityNormal = 4,
    ThreadPriorityHigh = 5,
    ThreadPriorityHigher = 6,
    ThreadPriorityHighest = 7
} ThreadPriority;

ThreadHandle tt_thread_alloc();
ThreadHandle tt_thread_alloc_ext(
    const char* name,
    uint32_t stackSize,
    ThreadCallback callback,
    void* _Nullable callbackContext
);
void tt_thread_free(ThreadHandle handle);
void tt_thread_set_name(ThreadHandle handle, const char* name);
void tt_thread_mark_as_static(ThreadHandle handle);
bool tt_thread_is_marked_as_static(ThreadHandle handle);
void tt_thread_set_stack_size(ThreadHandle handle, size_t size);
void tt_thread_set_callback(ThreadHandle handle, ThreadCallback callback, void* _Nullable callbackContext);
void tt_thread_set_priority(ThreadHandle handle, ThreadPriority priority);
void tt_thread_set_state_callback(ThreadHandle handle, ThreadStateCallback callback, void* _Nullable callbackContext);
ThreadState tt_thread_get_state(ThreadHandle handle);
void tt_thread_start(ThreadHandle handle);
bool tt_thread_join(ThreadHandle handle);
ThreadId tt_thread_get_id(ThreadHandle handle);
int32_t tt_thread_get_return_code(ThreadHandle handle);

#ifdef __cplusplus
}
#endif