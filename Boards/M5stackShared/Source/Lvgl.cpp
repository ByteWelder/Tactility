#include "esp_lvgl_port.h"
#include "Log.h"
#include "Thread.h"
#include "lvgl/LvglSync.h"

#define TAG "cores3_lvgl"

_Nullable lv_disp_t* createDisplay(bool usePsram);
_Nullable lv_indev_t* createTouch();

bool m5stack_lvgl_init() {
    static lv_display_t* display = nullptr;

    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = tt::Thread::PriorityHigh,
        .task_stack = 8096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "lvgl_port_init failed");
        return false;
    }

    // Set syncing functions
    tt::lvgl::syncSet(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}
