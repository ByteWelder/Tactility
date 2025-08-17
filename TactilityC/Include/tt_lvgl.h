#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** @return true if LVGL is started and active */
bool tt_lvgl_is_started();

/** Start LVGL and related background services */
void tt_lvgl_start();

/** Stop LVGL and related background services */
void tt_lvgl_stop();

#ifdef __cplusplus
}
#endif
