#include "hello_world.h"
#include "core_defines.h"

#include "esp_lvgl_port.h"
#include <esp_log.h>
#include "nb_hardware.h"

static const char* TAG = "app_helloworld";

static void prv_on_button_click(lv_event_t _Nonnull* event) {
    ESP_LOGI(TAG, "button clicked");
}

static int32_t prv_on_create(void* param) {
    UNUSED(param);
//    lvgl_port_lock(0);
//
//    lv_obj_t* label = lv_label_create(lv_parent);
//    lv_label_set_recolor(label, true);
//    lv_obj_set_width(label, (lv_coord_t)platform->display->horizontal_resolution);
//    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
//    lv_label_set_text(label, "Hello, world!");
//    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);
//
//    lv_obj_t* btn = lv_btn_create(lv_parent);
//    label = lv_label_create(btn);
//    lv_label_set_text_static(label, "Button");
//    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 30);
//    lv_obj_add_event_cb(btn, prv_on_button_click, LV_EVENT_CLICKED, NULL);
//
//    lvgl_port_unlock();
    return 0;
}

const nb_app_t hello_world_app = {
    .id = "helloworld",
    .name = "Hello World",
    .type = USER,
    .entry_point = &prv_on_create,
    .stack_size = 2048,
    .priority = 10
};
