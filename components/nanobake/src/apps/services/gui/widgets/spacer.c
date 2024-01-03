#include "spacer.h"
#include "widgets.h"

lv_obj_t* spacer(lv_obj_t* parent, lv_coord_t width, lv_coord_t height) {
    lv_obj_t* spacer = lv_obj_create(parent);
    lv_obj_set_size(spacer, width, height);
    lv_obj_set_style_bg_invisible(spacer);
    return spacer;
}
