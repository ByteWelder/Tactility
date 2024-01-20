#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*LvglLock)(int timeout_ticks);
typedef void (*LvglUnlock)();

void tt_lvgl_sync_set(LvglLock lock, LvglUnlock unlock);
bool tt_lvgl_lock(int timeout_ticks);
void tt_lvgl_unlock();

#ifdef __cplusplus
}
#endif
