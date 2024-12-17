#include "Spacer.h"
#include "Style.h"

namespace tt::lvgl {

[[deprecated("Use margin")]]
lv_obj_t* spacer_create(lv_obj_t* parent, int32_t width, int32_t height) {
    lv_obj_t* spacer = lv_obj_create(parent);
    lv_obj_set_size(spacer, width, height);
    obj_set_style_bg_invisible(spacer);
    return spacer;
}

} // namespace
