#include "tt_lvgl_spinner.h"
#include "lvgl/Spinner.h"

extern "C" {

lv_obj_t* tt_lvgl_spinner_create(lv_obj_t* parent) {
    return tt::lvgl::spinner_create(parent);
}

}
