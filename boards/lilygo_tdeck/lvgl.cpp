#include "config.h"
#include "display_i.h"
#include "esp_lvgl_port.h"
#include "keyboard.h"
#include "Log.h"
#include "ui/lvgl_sync.h"
#include "Thread.h"

#define TAG "tdeck_lvgl"

bool tdeck_init_touch(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle);

bool tdeck_init_lvgl() {
    static lv_disp_t* display = nullptr;
    static esp_lcd_panel_io_handle_t touch_io_handle;
    static esp_lcd_touch_handle_t touch_handle;

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = tt::THREAD_PRIORITY_RENDER,
        .task_stack = TDECK_LVGL_TASK_STACK_DEPTH,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    TT_LOG_D(TAG, "LVGL port init");
    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "LVGL port init failed");
        return false;
    }

    // Add display

    TT_LOG_D(TAG, "Creating display");
    display = tdeck_display_init();
    if (display == nullptr) {
        TT_LOG_E(TAG, "Creating display failed");
        return false;
    }

    // Add touch

    TT_LOG_D(TAG, "Creating touch");
    if (!tdeck_init_touch(&touch_io_handle, &touch_handle)) {
        TT_LOG_E(TAG, "Creating touch failed");
        return false;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = display,
        .handle = touch_handle,
    };

    TT_LOG_D(TAG, "Adding touch");
    lv_indev_t _Nullable* touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (touch_indev == nullptr) {
        TT_LOG_E(TAG, "Adding touch failed");
        return false;
    }

    // Set syncing functions
    tt::lvgl::sync_set(&lvgl_port_lock, &lvgl_port_unlock);

    keyboard_alloc(display);

    return true;
}
