#include "esp_lvgl_port.h"
#include "keyboard.h"
#include "log.h"
#include "ui/lvgl_sync.h"
#include <thread.h>

#define TAG "tdeck_lvgl"

lv_disp_t* tdeck_display_init();
void tdeck_enable_backlight();
bool tdeck_init_touch(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle);

bool tdeck_init_lvgl() {
    static lv_disp_t* display = NULL;
    static esp_lcd_panel_io_handle_t touch_io_handle;
    static esp_lcd_touch_handle_t touch_handle;

    // Init LVGL Port library

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = ThreadPriorityHigh,
        .task_stack = 8096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    TT_LOG_I(TAG, "lvgl port init");
    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "lvgl port init failed");
        return false;
    }

    // Add display

    TT_LOG_I(TAG, "creating display");
    display = tdeck_display_init();
    if (display == NULL) {
        TT_LOG_E(TAG, "creating display failed");
        return false;
    }

    // Add touch

    TT_LOG_I(TAG, "creating touch");
    if (!tdeck_init_touch(&touch_io_handle, &touch_handle)) {
        TT_LOG_E(TAG, "creating touch failed");
        return false;
    }

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = display,
        .handle = touch_handle,
    };

    TT_LOG_I(TAG, "adding touch");
    lv_indev_t _Nullable* touch_indev = lvgl_port_add_touch(&touch_cfg);
    if (touch_indev == NULL) {
        TT_LOG_E(TAG, "failed to add touch to lvgl");
        return false;
    }

    // Set syncing functions
    tt_lvgl_sync_set(&lvgl_port_lock, &lvgl_port_unlock);

    keyboard_alloc(display);

    TT_LOG_I(TAG, "enabling backlight");
    tdeck_enable_backlight();

    return true;
}
