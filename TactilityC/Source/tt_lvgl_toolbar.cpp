#include "tt_lvgl_toolbar.h"
#include <lvgl/Toolbar.h>

extern "C" {

lv_obj_t* tt_lvgl_toolbar_create(lv_obj_t* parent, AppHandle context) {
    return tt::lvgl::toolbar_create(parent, *(tt::app::AppContext*)context);
}

lv_obj_t* tt_lvgl_toolbar_create_simple(lv_obj_t* parent, const char* title) {
    return tt::lvgl::toolbar_create(parent, title);
}

}
