#include "hello_world.h"
#include <core_defines.h>
#include <record.h>
#include <nb_lvgl.h>
#include <nb_hardware.h>
#include <applications/services/gui/gui.h>
#include <esp_lvgl_port.h>
#include <esp_log.h>

static const char* TAG = "app_helloworld";

static void prv_on_button_click(lv_event_t _Nonnull* event) {
    ESP_LOGI(TAG, "button clicked");
    // Open Gui record
    struct NbGui* gui = furi_record_open(RECORD_GUI);

    // Free this screen
    ScreenId screen_id = (ScreenId)event->user_data;
    gui_screen_free(gui, screen_id);

    // Close Gui record
    furi_record_close(RECORD_GUI);
    gui = NULL;
}

static void prv_hello_world_lvgl(lv_obj_t* parent, ScreenId screen_id) {
    lvgl_port_lock(0);

    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_recolor(label, true);
    lv_obj_set_width(label, 200);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(label, "Hello, world!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t* btn = lv_btn_create(parent);
    label = lv_label_create(btn);
    lv_label_set_text_static(label, "Exit");
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 30);
    lv_obj_add_event_cb(btn, prv_on_button_click, LV_EVENT_CLICKED, (void*)screen_id);

    lvgl_port_unlock();

    // TODO: on app exit, call gui_screen_destroy()
}

static int32_t prv_hello_world_main(void* param) {
    UNUSED(param);

    // Open Gui record
    NbGuiHandle gui = furi_record_open(RECORD_GUI);

    // Register an lvgl screen
    gui_screen_create(gui, &prv_hello_world_lvgl);

    // Close Gui record
    furi_record_close(RECORD_GUI);
    gui = NULL;

    return 0;
}

const NbApp hello_world_app = {
    .id = "helloworld",
    .name = "Hello World",
    .type = USER,
    .entry_point = &prv_hello_world_main,
    .stack_size = 2048,
    .priority = 10
};
