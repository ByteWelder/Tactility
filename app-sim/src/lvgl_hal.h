#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void lvgl_task(void*);
void lvgl_interrupt();

bool lvgl_lock(int timeout_ticks);
void lvgl_unlock();

#ifdef __cplusplus
}
#endif