#include "hello_world.h"
#include "services/gui/gui.h"
#include "services/loader/loader.h"

static void app_show(TT_UNUSED App app, lv_obj_t* parent) {
    lv_obj_t* label = lv_label_create(parent);
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
    .on_show = &app_show,
    .on_hide = NULL
};
