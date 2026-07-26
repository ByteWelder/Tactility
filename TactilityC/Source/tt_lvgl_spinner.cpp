#include "tt_lvgl_spinner.h"
#include <lvgl/widgets/spinner.h>

extern "C" {

lv_obj_t* tt_lvgl_spinner_create(lv_obj_t* parent) {
    return lvgl_spinner_create(parent);
}

}
