#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long TickType;

#define TT_MAX_TICKS ((TickType)(~(TickType)0))

/**
 * Stall the current task for the specified amount of time.
 * @param milliseconds the time in milliseconds to stall.
 */
void tt_kernel_delay_millis(uint32_t milliseconds);

/**
 * Stall the current task for the specified amount of time.
 * @param milliseconds the time in microsends to stall.
 */
void tt_kernel_delay_micros(uint32_t microSeconds);

/**
 * Stall the current task for the specified amount of time.
 * @param milliseconds the time in ticks to stall.
 */
void tt_kernel_delay_ticks(TickType ticks);

/** @return the number of ticks since the device was started */
TickType tt_kernel_get_ticks();

/** Convert milliseconds to ticks */
TickType tt_kernel_millis_to_ticks(uint32_t milliSeconds);

/** Stall the current task until the specified timestamp
 * @return false if for some reason the delay was broken off
 */
bool tt_kernel_delay_until_tick(TickType tick);

/** @return the tick frequency of the kernel (commonly 1000 Hz when running FreeRTOS) */
uint32_t tt_kernel_get_tick_frequency();

/** @return the number of milliseconds that have passed since the device was started */
uint32_t tt_kernel_get_millis();

unsigned long tt_kernel_get_micros();

#ifdef __cplusplus
}
#endif
