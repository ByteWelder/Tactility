#include "system_info.h"

#include <esp_lvgl_port.h>
#include <nb_platform.h>

static void prv_on_create(nb_platform_t _Nonnull* platform, lv_obj_t _Nonnull* lv_parent) {
    lvgl_port_lock(0);

    lv_obj_t* cpu_label = lv_label_create(lv_parent);
    lv_label_set_recolor(cpu_label, true);
    lv_obj_set_width(cpu_label, (lv_coord_t)platform->display->horizontal_resolution);
    lv_obj_set_style_text_align(cpu_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(cpu_label, "CPU usage: ?");
    lv_obj_align(cpu_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* mem_free_label = lv_label_create(lv_parent);
    lv_label_set_recolor(mem_free_label, true);
    lv_obj_set_width(mem_free_label, (lv_coord_t)platform->display->horizontal_resolution);
    lv_obj_set_style_text_align(mem_free_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_text(mem_free_label, "Memory: ?");
    lv_obj_align(mem_free_label, LV_ALIGN_TOP_LEFT, 0, 15);

    lvgl_port_unlock();
}

nb_app_config_t system_info_app_config() {
    nb_app_config_t config = {
        .id = "systeminfo",
        .name = "System Info",
        .type = SYSTEM,
        .on_create = &prv_on_create,
        .on_update = NULL,
        .on_destroy = NULL
    };
    return config;
}
