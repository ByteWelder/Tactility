#pragma once

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include <esp_timer.h>
#else
#include "FreeRTOS.h"
#include <sys/time.h>
#endif

namespace tt::kernel {

/** Recognized platform types */
typedef enum {
    PlatformEsp,
    PlatformSimulator
} Platform;

/** Return true when called from an Interrupt Service Routine (~IRQ mode) */
#ifdef ESP_PLATFORM
constexpr bool isIsr() { return (xPortInIsrContext() == pdTRUE); }
#else
constexpr bool isIsr() { return false; }
#endif

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

constexpr size_t getMillis() { return getTicks() / portTICK_PERIOD_MS; }

constexpr long int getMicros() {
#ifdef ESP_PLATFORM
    return static_cast<unsigned long>(esp_timer_get_time());
#else
    timeval tv;
    gettimeofday(&tv, nullptr);
    return 1000000 * tv.tv_sec + tv.tv_usec;
#endif
}

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

constexpr TickType_t secondsToTicks(uint32_t seconds) {
    return static_cast<TickType_t>(seconds) * 1000U / portTICK_PERIOD_MS;
}

constexpr TickType_t minutesToTicks(uint32_t minutes) {
    return secondsToTicks(minutes * 60U);
}

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
