#pragma once

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#endif

namespace tt::kernel {

/** Recognized platform types */
typedef enum {
    PlatformEsp,
    PlatformSimulator
} Platform;

/** Check if kernel is running
 * @return true if the FreeRTOS kernel is running, false otherwise
 */
bool isRunning();

/** Lock kernel, pause process scheduling
 * @warning don't call from ISR context
 * @return true on success
 */
bool lock();

/** Unlock kernel, resume process scheduling
 * @warning don't call from ISR context
 * @return true on success
 */
bool unlock();

/** Restore kernel lock state
 * @warning don't call from ISR context
 * @param[in] lock  The lock state
 * @return true on success
 */
bool restoreLock(bool lock);

/** Get kernel systick frequency
 * @return systick counts per second
 */
uint32_t getTickFrequency();

TickType_t getTicks();

/** Delay execution
 * @warning don't call from ISR context
 * Also keep in mind delay is aliased to scheduler timer intervals.
 * @param[in] ticks The ticks count to pause
 */
void delayTicks(TickType_t ticks);

/** Delay until tick
 * @warning don't call from ISR context
 * @param[in] ticks  The tick until which kerel should delay task execution
 * @return true on success
 */
bool delayUntilTick(TickType_t tick);

/** Convert milliseconds to ticks
 *
 * @param[in]   milliSeconds    time in milliseconds
 * @return      time in ticks
 */
TickType_t millisToTicks(uint32_t milliSeconds);

/** Delay in milliseconds
 * This method uses kernel ticks on the inside, which causes delay to be aliased to scheduler timer intervals.
 * Real wait time will be between X+ milliseconds.
 * Special value: 0, will cause task yield.
 * Also if used when kernel is not running will fall back to delayMicros()
 * @warning don't call from ISR context
 * @param[in] milliSeconds milliseconds to wait
 */
void delayMillis(uint32_t milliSeconds);

/** Delay in microseconds
 * Implemented using Cortex DWT counter. Blocking and non aliased.
 * @param[in] microSeconds microseconds to wait
 */
void delayMicros(uint32_t microSeconds);

/** @return the platform that Tactility currently is running on. */
Platform getPlatform();

} // namespace
