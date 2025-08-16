#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long TickType;

void tt_kernel_delay_millis(uint32_t milliseconds);

void tt_kernel_delay_micros(uint32_t microSeconds);

void tt_kernel_delay_ticks(TickType ticks);

TickType tt_kernel_get_ticks();

TickType tt_kernel_millis_to_ticks(uint32_t milliSeconds);

bool tt_kernel_delay_until_tick(TickType tick);

uint32_t tt_kernel_get_tick_frequency();

uint32_t tt_kernel_get_millis();

unsigned long tt_kernel_get_micros();

#ifdef __cplusplus
}
#endif
