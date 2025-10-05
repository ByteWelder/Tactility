#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @return true if LVGL is started and active */
bool tt_lvgl_is_started();

/** Start LVGL and related background services */
void tt_lvgl_start();

/** Stop LVGL and related background services */
void tt_lvgl_stop();

/** Lock the LVGL context. Call this before doing LVGL-related operations from a non-LVLG thread */
void tt_lvgl_lock();

/** Unlock the LVGL context */
void tt_lvgl_unlock();

#ifdef __cplusplus
}
#endif
