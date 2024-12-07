#pragma once

#include "CoreTypes.h"

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#endif

namespace tt::kernel {

typedef enum {
    PlatformEsp,
    PlatformSimulator
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
bool isIrq();

/** Check if kernel is running
 *
 * @return     true if running, false otherwise
 */
bool isRunning();

/** Lock kernel, pause process scheduling
 *
 * @warning This should never be called in interrupt request context.
 *
 * @return     previous lock state(0 - unlocked, 1 - locked)
 */
int32_t lock();

/** Unlock kernel, resume process scheduling
 *
 * @warning This should never be called in interrupt request context.
 *
 * @return     previous lock state(0 - unlocked, 1 - locked)
 */
int32_t unlock();

/** Restore kernel lock state
 *
 * @warning This should never be called in interrupt request context.
 *
 * @param[in]  lock  The lock state
 *
 * @return     new lock state or error
 */
int32_t restoreLock(int32_t lock);

/** Get kernel systick frequency
 *
 * @return     systick counts per second
 */
uint32_t getTickFrequency();

TickType_t getTicks();

/** Delay execution
 *
 * @warning This should never be called in interrupt request context.
 *
 * Also keep in mind delay is aliased to scheduler timer intervals.
 *
 * @param[in]  ticks  The ticks count to pause
 */
void delayTicks(TickType_t ticks);

/** Delay until tick
 *
 * @warning This should never be called in interrupt request context.
 *
 * @param[in]  ticks  The tick until which kerel should delay task execution
 *
 * @return     The status.
 */
TtStatus delayUntilTick(uint32_t tick);

/** Convert milliseconds to ticks
 *
 * @param[in]   milliSeconds    time in milliseconds
 * @return      time in ticks
 */
TickType_t millisToTicks(uint32_t milliSeconds);

/** Delay in milliseconds
 * 
 * This method uses kernel ticks on the inside, which causes delay to be aliased to scheduler timer intervals.
 * Real wait time will be between X+ milliseconds.
 * Special value: 0, will cause task yield.
 * Also if used when kernel is not running will fall back to `tt_delay_us`.
 * 
 * @warning    Cannot be used from ISR
 *
 * @param[in]  milliSeconds  milliseconds to wait
 */
void delayMillis(uint32_t milliSeconds);

/** Delay in microseconds
 *
 * Implemented using Cortex DWT counter. Blocking and non aliased.
 *
 * @param[in]  microSeconds  microseconds to wait
 */
void delayMicros(uint32_t microSeconds);

Platform getPlatform();

} // namespace
