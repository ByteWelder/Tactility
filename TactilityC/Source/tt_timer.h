#pragma once

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void* TimerHandle;

typedef enum {
    TimerTypeOnce = 0,    ///< One-shot timer.
    TimerTypePeriodic = 1 ///< Repeating timer.
} TimerType;

typedef enum {
    TimerThreadPriorityNormal,   /**< Lower then other threads */
    TimerThreadPriorityElevated, /**< Same as other threads */
} TimerThreadPriority;

typedef void (*TimerCallback)(void* context);
typedef void (*TimerPendingCallback)(void* context, uint32_t arg);

TimerHandle tt_timer_alloc(TimerType type, TimerCallback callback, void* callbackContext);
void tt_timer_free(TimerHandle handle);
bool tt_timer_start(TimerHandle handle, TickType_t intervalTicks);
bool tt_timer_restart(TimerHandle handle, TickType_t intervalTicks);
bool tt_timer_stop(TimerHandle handle);
bool tt_timer_is_running(TimerHandle handle);
uint32_t tt_timer_get_expire_time(TimerHandle handle);
bool tt_timer_set_pending_callback(TimerHandle handle, TimerPendingCallback callback, void* callbackContext, uint32_t callbackArg, TickType_t timeoutTicks);
void tt_timer_set_thread_priority(TimerHandle handle, TimerThreadPriority priority);

#ifdef __cplusplus
}
#endif