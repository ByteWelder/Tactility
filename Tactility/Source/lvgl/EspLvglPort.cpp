#ifdef ESP_PLATFORM

#include <Tactility/Thread.h>
#include <Tactility/CpuAffinity.h>
#include <Tactility/Logger.h>
#include <Tactility/Mutex.h>
#include <Tactility/lvgl/LvglSync.h>

#include <esp_lvgl_port.h>

namespace tt::lvgl {

// LVGL
// The minimum task stack seems to be about 3500, but that crashes the wifi app in some scenarios
// At 8192, it sometimes crashes when wifi-auto enables and is busy connecting and then you open WifiManage
constexpr auto LVGL_TASK_STACK_DEPTH = 9216;

static const auto LOGGER = Logger("EspLvglPort");

bool initEspLvglPort() {
    LOGGER.debug("Init");
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = static_cast<UBaseType_t>(Thread::Priority::Critical),
        .task_stack = LVGL_TASK_STACK_DEPTH,
        .task_affinity = getCpuAffinityConfiguration().graphics,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };

    if (lvgl_port_init(&lvgl_cfg) != ESP_OK) {
        LOGGER.error("Init failed");
        return false;
    }

    syncSet(&lvgl_port_lock, &lvgl_port_unlock);

    return true;
}

} // namespace tt::lvgl

#endif
