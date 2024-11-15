#include "hello_world.h"
#include "services/loader/loader.h"
#include "ui/toolbar.h"

static void app_show(App app, lv_obj_t* parent) {
    lv_obj_t* toolbar = tt_toolbar_create_for_app(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

extern const AppManifest hello_world_app = {
    .id = "helloworld",
    .name = "Hello World",
    .icon = nullptr,
    .type = AppTypeUser,
    .on_start = nullptr,
    .on_stop = nullptr,
    .on_show = &app_show,
    .on_hide = __null
};
