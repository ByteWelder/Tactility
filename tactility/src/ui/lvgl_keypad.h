#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

bool tt_lvgl_keypad_is_available();
void tt_lvgl_keypad_set_indev(lv_indev_t* device);
void tt_lvgl_keypad_activate(lv_group_t* group);
void tt_lvgl_keypad_deactivate();

#ifdef __cplusplus
}
#endif