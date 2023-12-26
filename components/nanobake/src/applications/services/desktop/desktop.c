#include "desktop.h"
#include "nb_hardware.h"
#include <esp_lvgl_port.h>
#include "core_defines.h"
#include <esp_log.h>

//nb_desktop_t* shared_desktop = NULL;

static int32_t prv_desktop_main(void* param) {
    UNUSED(param);
    printf("desktop app init\n");
//    nb_desktop_t* desktop = desktop_alloc();
//    shared_desktop = desktop;

//    lvgl_port_lock(0);
//
//    lv_obj_t* label = lv_label_create(lv_parent);
//    lv_label_set_recolor(label, true);
//    lv_obj_set_width(label, (lv_coord_t)hardware->display->horizontal_resolution);
//    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
//    lv_label_set_text(label, "Desktop app");
//    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
//
//    lvgl_port_unlock();
    return 0;
}

const nb_app_t desktop_app = {
    .id = "desktop",
    .name = "Desktop",
    .type = SERVICE,
    .entry_point = &prv_desktop_main,
    .stack_size = 2048,
    .priority = 10
};
