#include "esp_lvgl_port.h"
#include "log.h"
#include "ui/lvgl_sync.h"
#include <thread.h>

#define TAG "lilygo_tdeck_lvgl"

lv_disp_t* lilygo_tdeck_init_display();
bool lilygo_tdeck_init_touch(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle);

static bool lvgl_lock_impl(int timeout_ticks) {
    return lvgl_port_lock(timeout_ticks);
}

static void lvgl_unlock_impl() {
    lvgl_port_unlock();
}

bool lilygo_init_lvgl() {
    static lv_disp_t* display = NULL;
    static esp_lcd_panel_io_handle_t touch_io_handle;
    static esp_lcd_touch_handle_t touch_handle;

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = ThreadPriorityHigh,
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
    display = lilygo_tdeck_init_display();
    if (display == NULL) {
        TT_LOG_E(TAG, "failed to add display");
        return false;
    }


    // Add touch
    if (!lilygo_tdeck_init_touch(&touch_io_handle, &touch_handle)) {
        return false;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = display,
        .handle = touch_handle,
    };

    lv_indev_t _Nullable* touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (touch_indev == NULL) {
        TT_LOG_E(TAG, "failed to add touch to lvgl");
        return false;
    }

    // Set syncing functions
    tt_lvgl_sync_set(&lvgl_lock_impl, &lvgl_unlock_impl);

    return true;
}
