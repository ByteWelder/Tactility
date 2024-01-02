#include "hello_world.h"
#include "furi.h"
#include "apps/services/gui/gui.h"
#include "apps/services/loader/loader.h"

static void on_button_click(lv_event_t _Nonnull* event) {
    FURI_RECORD_TRANSACTION(RECORD_LOADER, Loader*, loader, {
        loader_start_app_nonblocking(loader, "systeminfo", NULL);
    })
}

static void app_show(lv_obj_t* parent, void* context) {
    UNUSED(context);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_recolor(label, true);
    lv_obj_set_width(label, 200);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t* btn = lv_btn_create(parent);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, "System Info");
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(btn, on_button_click, LV_EVENT_CLICKED, NULL);
}

const AppManifest hello_world_app = {
    .id = "helloworld",
    .name = "Hello World",
    .icon = NULL,
    .type = AppTypeUser,
    .on_start = NULL,
    .on_stop = NULL,
    .on_show = &app_show,
    .stack_size = AppStackSizeNormal,
};
