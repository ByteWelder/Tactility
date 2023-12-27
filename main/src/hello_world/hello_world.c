#include "hello_world.h"
#include "core_defines.h"
#include "record.h"
#include "nb_lvgl.h"
#include "nb_hardware.h"
#include "applications/services/gui/gui.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char* TAG = "app_helloworld";

ViewPort* view_port = NULL;

static void prv_on_button_click(lv_event_t _Nonnull* event) {
    ESP_LOGI(TAG, "button clicked");

    // TODO: make macro for record 'transactions'
    NbGui* gui = furi_record_open(RECORD_GUI);
    gui_remove_view_port(gui, view_port);

    view_port_free(view_port);
    view_port = NULL;

    // Close Gui record
    furi_record_close(RECORD_GUI);
    gui = NULL;
}

static void prv_hello_world_lvgl(lv_obj_t* parent, void* context) {
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
    lv_obj_add_event_cb(btn, prv_on_button_click, LV_EVENT_CLICKED, NULL);

    lvgl_port_unlock();
}

static int32_t prv_hello_world_main(void* param) {
    UNUSED(param);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Configure view port
    view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, &prv_hello_world_lvgl, view_port);

    // Register view port in GUI
    NbGui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

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
