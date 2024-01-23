#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*LvglLock)(uint32_t timeout_ticks);
typedef void (*LvglUnlock)();

void tt_lvgl_sync_set(LvglLock lock, LvglUnlock unlock);
bool tt_lvgl_lock(uint32_t timeout_ticks);
void tt_lvgl_unlock();

#ifdef __cplusplus
}
#endif
