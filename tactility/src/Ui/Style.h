#pragma once

#include "lvgl.h"

namespace tt::lvgl {

void obj_set_style_bg_blacken(lv_obj_t* obj);
void obj_set_style_bg_invisible(lv_obj_t* obj);
void obj_set_style_no_padding(lv_obj_t* obj);

/**
 * This is to create automatic padding depending on the screen size.
 * The larger the screen, the more padding it gets.
 * TODO: It currently only applies a single basic padding, but will be improved later.
 *
 * @param obj
 */
void obj_set_style_auto_padding(lv_obj_t* obj);

} // namespace
