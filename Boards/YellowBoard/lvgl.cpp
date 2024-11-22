#include "esp_lvgl_port.h"
#include "Log.h"
#include "Ui/LvglSync.h"
#include "Thread.h"

#define TAG "twodotfour_lvgl"

lv_display_t* twodotfour_display_init();
bool twodotfour_touch_init(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle);

bool twodotfour_lvgl_init() {
    static lv_display_t* display = nullptr;
    static esp_lcd_panel_io_handle_t touch_io_handle;
    static esp_lcd_touch_handle_t touch_handle;

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = tt::Thread::PriorityHigh,
        .task_stack = 8096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "lvgl port init failed");
        return false;
    }

    // Add display
    display = twodotfour_display_init();
    if (display == nullptr) {
        TT_LOG_E(TAG, "failed to add display");
        return false;
    }

    // Add touch
    if (!twodotfour_touch_init(&touch_io_handle, &touch_handle)) {
        return false;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = display,
        .handle = touch_handle,
    };

    auto* touch_indev= lvgl_port_add_touch(&touch_cfg);
    if (touch_indev == nullptr) {
        TT_LOG_E(TAG, "failed to add touch to lvgl");
        return false;
    }

    // Set syncing functions
    tt::lvgl::sync_set(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}
