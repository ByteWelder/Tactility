#include "esp_lvgl_port.h"
#include <Tactility/Log.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/Thread.h>

#define TAG "twodotfour_lvgl"

bool twodotfour_lvgl_init() {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = static_cast<UBaseType_t>(tt::THREAD_PRIORITY_RENDER),
        .task_stack = 8096,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "lvgl port init failed");
        return false;
    }

    // Set syncing functions
    tt::lvgl::syncSet(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}
