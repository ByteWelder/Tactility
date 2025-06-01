#pragma once

#include <lvgl.h>

namespace tt::lvgl {

void obj_set_style_bg_blacken(lv_obj_t* obj);
void obj_set_style_bg_invisible(lv_obj_t* obj);

[[deprecated("use _pad_all() and _pad_gap() individually")]]
void obj_set_style_no_padding(lv_obj_t* obj);

} // namespace
