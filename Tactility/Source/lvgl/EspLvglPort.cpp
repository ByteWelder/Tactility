#ifdef ESP_PLATFORM

#include <Tactility/lvgl/LvglSync.h>
#include <esp_lvgl_port.h>
#include <Tactility/Mutex.h>

// LVGL
// The minimum task stack seems to be about 3500, but that crashes the wifi app in some scenarios
// At 8192, it sometimes crashes when wifi-auto enables and is busy connecting and then you open WifiManage
#define TDECK_LVGL_TASK_STACK_DEPTH 9216
#define TAG "lvgl"

namespace tt::lvgl {

bool initEspLvglPort() {
    TT_LOG_D(TAG, "Port init");
    static lv_disp_t* display = nullptr;
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = static_cast<UBaseType_t>(Thread::Priority::Critical),
        .task_stack = TDECK_LVGL_TASK_STACK_DEPTH,
        .task_affinity = 1, // -1 = disabled, 0 = core 1, 1 = core 2
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        TT_LOG_E(TAG, "Port init failed");
        return false;
    }

    tt::lvgl::syncSet(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}

} // namespace tt::lvgl

#endif
