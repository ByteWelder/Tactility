#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void tt_lv_obj_set_style_bg_blacken(lv_obj_t* obj);
void tt_lv_obj_set_style_bg_invisible(lv_obj_t* obj);
void tt_lv_obj_set_style_no_padding(lv_obj_t* obj);

/**
 * This is to create automatic padding depending on the screen size.
 * The larger the screen, the more padding it gets.
 * TODO: It currently only applies a single basic padding, but will be improved later.
 *
 * @param obj
 */
void tt_lv_obj_set_style_auto_padding(lv_obj_t* obj);

#ifdef __cplusplus
}
#endif
