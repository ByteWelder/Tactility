#pragma once

#include "lvgl.h"

namespace tt::lvgl {

[[deprecated("Use margin")]]
lv_obj_t* spacer_create(lv_obj_t* parent, int32_t width, int32_t height);

} // namespace
