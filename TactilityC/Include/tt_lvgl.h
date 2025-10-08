#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TT_LVGL_DEFAULT_LOCK_TIME 500 // 500 ticks = 500 ms

/** @return true if LVGL is started and active */
bool tt_lvgl_is_started();

/** Start LVGL and related background services */
void tt_lvgl_start();

/** Stop LVGL and related background services */
void tt_lvgl_stop();

/** Lock the LVGL context. Call this before doing LVGL-related operations from a non-LVLG thread */
bool tt_lvgl_lock(TickType timeout);

/** Unlock the LVGL context */
void tt_lvgl_unlock();

#ifdef __cplusplus
}
#endif
