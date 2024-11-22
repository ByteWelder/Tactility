#pragma once

#include "CoreTypes.h"

namespace tt {

typedef enum {
    PlatformEsp,
    PlatformPc
} Platform;

/** Check if CPU is in IRQ or kernel running and IRQ is masked
 * 
 * Originally this primitive was born as a workaround for FreeRTOS kernel primitives shenanigans with PRIMASK.
 * 
 * Meaningful use cases are:
 * 
 * - When kernel is started and you want to ensure that you are not in IRQ or IRQ is not masked(like in critical section)
 * - When kernel is not started and you want to make sure that you are not in IRQ mode, ignoring PRIMASK.
 * 
 * As you can see there will be edge case when kernel is not started and PRIMASK is not 0 that may cause some funky behavior.
 * Most likely it will happen after kernel primitives being used, but control not yet passed to kernel.
 * It's up to you to figure out if it is safe for your code or not.
 * 
 * @return     true if CPU is in IRQ or kernel running and IRQ is masked
 */
bool kernel_is_irq();

/** Check if kernel is running
 *
 * @return     true if running, false otherwise
 */
bool kernel_is_running();

/** Lock kernel, pause process scheduling
 *
 * @warning This should never be called in interrupt request context.
 *
 * @return     previous lock state(0 - unlocked, 1 - locked)
 */
int32_t kernel_lock();

/** Unlock kernel, resume process scheduling
 *
 * @warning This should never be called in interrupt request context.
 *
 * @return     previous lock state(0 - unlocked, 1 - locked)
 */
int32_t kernel_unlock();

/** Restore kernel lock state
 *
 * @warning This should never be called in interrupt request context.
 *
 * @param[in]  lock  The lock state
 *
 * @return     new lock state or error
 */
int32_t kernel_restore_lock(int32_t lock);

/** Get kernel systick frequency
 *
 * @return     systick counts per second
 */
uint32_t kernel_get_tick_frequency();

/** Delay execution
 *
 * @warning This should never be called in interrupt request context.
 *
 * Also keep in mind delay is aliased to scheduler timer intervals.
 *
 * @param[in]  ticks  The ticks count to pause
 */
void delay_tick(uint32_t ticks);

/** Delay until tick
 *
 * @warning This should never be called in interrupt request context.
 *
 * @param[in]  ticks  The tick until which kerel should delay task execution
 *
 * @return     The status.
 */
TtStatus delay_until_tick(uint32_t tick);

/** Convert milliseconds to ticks
 *
 * @param[in]   milliseconds    time in milliseconds
 * @return      time in ticks
 */
uint32_t ms_to_ticks(uint32_t milliseconds);

/** Delay in milliseconds
 * 
 * This method uses kernel ticks on the inside, which causes delay to be aliased to scheduler timer intervals.
 * Real wait time will be between X+ milliseconds.
 * Special value: 0, will cause task yield.
 * Also if used when kernel is not running will fall back to `tt_delay_us`.
 * 
 * @warning    Cannot be used from ISR
 *
 * @param[in]  milliseconds  milliseconds to wait
 */
void delay_ms(uint32_t milliseconds);

/** Delay in microseconds
 *
 * Implemented using Cortex DWT counter. Blocking and non aliased.
 *
 * @param[in]  microseconds  microseconds to wait
 */
void delay_us(uint32_t microseconds);

Platform get_platform();

} // namespace
