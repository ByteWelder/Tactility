#include "esp_lvgl_port.h"
#include "log.h"
#include "thread.h"
#include "ui/lvgl_sync.h"

#define TAG "cores3_lvgl"

extern _Nullable lv_disp_t* m5stack_lvgl_display(bool usePsram);
extern _Nullable lv_indev_t* m5stack_lvgl_touch();

bool m5stack_lvgl_init() {
    static lv_display_t* display = NULL;

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = ThreadPriorityHigh,
        .task_stack = 8096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "lvgl_port_init failed");
        return false;
    }

    // Add display
    display = m5stack_lvgl_display(true);
    if (display == NULL) {
        TT_LOG_E(TAG, "failed to add display");
        return false;
    }

    // Add touch
    lv_indev_t* touch_indev = m5stack_lvgl_touch();
    if (touch_indev == NULL) {
        TT_LOG_E(TAG, "failed to add touch");
        return false;
    }
    lv_indev_set_display(touch_indev, display);

    // Set syncing functions
    tt_lvgl_sync_set(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}
