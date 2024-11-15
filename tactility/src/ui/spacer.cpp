#include "spacer.h"
#include "style.h"

lv_obj_t* tt_lv_spacer_create(lv_obj_t* parent, int32_t width, int32_t height) {
    lv_obj_t* spacer = lv_obj_create(parent);
    lv_obj_set_size(spacer, width, height);
    tt_lv_obj_set_style_bg_invisible(spacer);
    return spacer;
}
