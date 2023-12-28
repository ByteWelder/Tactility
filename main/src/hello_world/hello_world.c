#include "hello_world.h"
#include "record.h"
#include "nb_lvgl.h"
#include "applications/services/gui/gui.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char* TAG = "app_helloworld";

ViewPort* view_port = NULL;

static void on_button_click(lv_event_t _Nonnull* event) {
    ESP_LOGI(TAG, "button clicked");

    FURI_RECORD_TRANSACTION(RECORD_GUI, gui, {
        gui_remove_view_port(gui, view_port);
        view_port_free(view_port);
        view_port = NULL;
    });
}

// Main entry point for LVGL widget creation
static void app_lvgl(lv_obj_t* parent, void* context) {
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
    lv_obj_add_event_cb(btn, on_button_click, LV_EVENT_CLICKED, NULL);

    lvgl_port_unlock();
}

// Main entry point for the app
static int32_t app_main(void* param) {
    UNUSED(param);

    // Configure view port to enable UI with LVGL
    view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, &app_lvgl, view_port);

    // The transaction automatically calls furi_record_open() and furi_record_close()
    FURI_RECORD_TRANSACTION(RECORD_GUI, gui, {
        gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    })

    return 0;
}

const NbApp hello_world_app = {
    .id = "helloworld",
    .name = "Hello World",
    .type = USER,
    .entry_point = &app_main,
    .stack_size = NB_TASK_STACK_SIZE_DEFAULT,
    .priority = NB_TASK_PRIORITY_DEFAULT
};
