#include "display_i.hpp"
#include "esp_lvgl_port.h"
#include "log.h"
#include "thread.h"
#include "touch_i.hpp"
#include "ui/lvgl_sync.h"

#define TAG "cores3_lvgl"

bool cores3_lvgl_init() {
    static lv_display_t* display = NULL;

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
    display = cores3_display_init();
    if (display == NULL) {
        TT_LOG_E(TAG, "failed to add display");
        return false;
    }

    // Add touch
    if (!cores3_touch_init()) {
        return false;
    }

    // Set syncing functions
    tt_lvgl_sync_set(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}
