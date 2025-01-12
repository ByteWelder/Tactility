#include "Log.h"
#include "Thread.h"
#include "lvgl/LvglSync.h"
#include "esp_lvgl_port.h"

#define TAG "core2"

// LVGL
// The minimum task stack seems to be about 3500, but that crashes the wifi app in some scenarios
// At 4000, it crashes when the fps renderer is available
#define CORE2_LVGL_TASK_STACK_DEPTH 9216

bool initLvgl() {
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = static_cast<UBaseType_t>(tt::THREAD_PRIORITY_RENDER),
        .task_stack = CORE2_LVGL_TASK_STACK_DEPTH,
        .task_affinity = -1, // core pinning
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    TT_LOG_D(TAG, "LVGL port init");
    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "LVGL port init failed");
        return false;
    }

    tt::lvgl::syncSet(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}
