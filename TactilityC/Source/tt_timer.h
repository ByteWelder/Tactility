#pragma once

#include "tt_thread.h"
#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** The handle that represents a timer instance */
typedef void* TimerHandle;

/** The behaviour of the timer */
typedef enum {
    TimerTypeOnce = 0,    ///< One-shot timer.
    TimerTypePeriodic = 1 ///< Repeating timer.
} TimerType;

typedef void (*TimerCallback)(void* context);
typedef void (*TimerPendingCallback)(void* context, uint32_t arg);

/**
 * Create a new timer instance
 * @param[in] callback the callback to call when the timer expires
 * @param[in] callbackContext the data to pass to the callback
 */
TimerHandle tt_timer_alloc(TimerType type, TimerCallback callback, void* callbackContext);

/** Free up the memory of a timer instance */
void tt_timer_free(TimerHandle handle);

/**
 * Start the timer
 * @param[in] handle the timer instance handle
 * @parma[in] interval the interval of the timer
 * @return true when the timer was successfully started
 */
bool tt_timer_start(TimerHandle handle, TickType_t interval);

/**
 * Restart an already started timer
 * @param[in] handle the timer instance handle
 * @parma[in] interval the interval of the timer
 * @return true when the timer was successfully restarted
 */
bool tt_timer_restart(TimerHandle handle, TickType_t interval);

/**
 * Stop a started timer
 * @param[in] handle the timer instance handle
 * @return true when the timer was successfully stopped
 */
bool tt_timer_stop(TimerHandle handle);

/**
 * Check if a timer is started
 * @param[in] handle the timer instance handle
 * @return true when the timer is started (pending)
 */
bool tt_timer_is_running(TimerHandle handle);

/**
 * Get the expire time of a timer
 * @param[in] handle the timer instance handle
 * @return the absolute timestamp at which the timer will expire
 */
uint32_t tt_timer_get_expire_time(TimerHandle handle);

/**
 * Set the pending callback for a timer
 * @param[in] handle the timer instance handle
 * @param[in] callback the callback to set
 * @param[in] callbackContext the context to pass to the callback
 * @param[in] timeout the timeout for setting the callback
 * @return when the callback was successfully set
 */
bool tt_timer_set_pending_callback(TimerHandle handle, TimerPendingCallback callback, void* callbackContext, uint32_t callbackArg, TickType_t timeout);

/**
 * Set the thread priority for the callback of the timer
 * @param[in] handle the timer instance handle
 * @param[in] priority the thread priority to set
 */
void tt_timer_set_thread_priority(TimerHandle handle, ThreadPriority priority);

#ifdef __cplusplus
}
#endif