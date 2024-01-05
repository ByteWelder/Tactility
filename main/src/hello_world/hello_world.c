#include "hello_world.h"
#include "apps/services/gui/gui.h"
#include "apps/services/loader/loader.h"

static void app_show(lv_obj_t* parent, void* context) {
    UNUSED(context);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_recolor(label, true);
    lv_obj_set_width(label, 200);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

const AppManifest hello_world_app = {
    .id = "helloworld",
    .name = "Hello World",
    .icon = NULL,
    .type = AppTypeUser,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show
};
