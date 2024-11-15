#pragma once

#include "core_types.h"

typedef void (*TimerCallback)(void* context);

typedef enum {
    TimerTypeOnce = 0,    ///< One-shot timer.
    TimerTypePeriodic = 1 ///< Repeating timer.
} TimerType;

typedef void Timer;

/** Allocate timer
 *
 * @param[in]  func     The callback function
 * @param[in]  type     The timer type
 * @param      context  The callback context
 *
 * @return     The pointer to Timer instance
 */
Timer* tt_timer_alloc(TimerCallback func, TimerType type, void* context);

/** Free timer
 *
 * @param      instance  The pointer to Timer instance
 */
void tt_timer_free(Timer* instance);

/** Start timer
 *
 * @warning    This is asynchronous call, real operation will happen as soon as
 *             timer service process this request.
 *
 * @param      instance  The pointer to Timer instance
 * @param[in]  ticks     The interval in ticks
 *
 * @return     The status.
 */
TtStatus tt_timer_start(Timer* instance, uint32_t ticks);

/** Restart timer with previous timeout value
 *
 * @warning    This is asynchronous call, real operation will happen as soon as
 *             timer service process this request.
 *
 * @param      instance  The pointer to Timer instance
 * @param[in]  ticks     The interval in ticks
 *
 * @return     The status.
 */
TtStatus tt_timer_restart(Timer* instance, uint32_t ticks);

/** Stop timer
 *
 * @warning    This is asynchronous call, real operation will happen as soon as
 *             timer service process this request.
 *
 * @param      instance  The pointer to Timer instance
 *
 * @return     The status.
 */
TtStatus tt_timer_stop(Timer* instance);

/** Is timer running
 *
 * @warning    This cal may and will return obsolete timer state if timer
 *             commands are still in the queue. Please read FreeRTOS timer
 *             documentation first.
 *
 * @param      instance  The pointer to Timer instance
 *
 * @return     0: not running, 1: running
 */
uint32_t tt_timer_is_running(Timer* instance);

/** Get timer expire time
 *
 * @param      instance  The Timer instance
 *
 * @return     expire tick
 */
uint32_t tt_timer_get_expire_time(Timer* instance);

typedef void (*TimerPendigCallback)(void* context, uint32_t arg);

void tt_timer_pending_callback(TimerPendigCallback callback, void* context, uint32_t arg);

typedef enum {
    TimerThreadPriorityNormal,   /**< Lower then other threads */
    TimerThreadPriorityElevated, /**< Same as other threads */
} TimerThreadPriority;

/** Set Timer thread priority
 *
 * @param[in]  priority  The priority
 */
void tt_timer_set_thread_priority(TimerThreadPriority priority);
