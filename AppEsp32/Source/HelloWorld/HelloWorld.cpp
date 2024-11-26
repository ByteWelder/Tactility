#include "lvgl.h"
#include "lvgl/Toolbar.h"

static void app_show(tt::app::App& app, lv_obj_t* parent) {
    lv_obj_t* toolbar = tt::lvgl::toolbar_create(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

extern const tt::app::Manifest hello_world_app = {
    .id = "HelloWorld",
    .name = "Hello World",
    .type = tt::app::TypeUser,
    .onShow = &app_show,
};
