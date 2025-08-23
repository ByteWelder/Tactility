#include "Tactility/CpuAffinity.h"

#include <Tactility/Check.h>

namespace tt {

#ifdef ESP_PLATFORM

static CpuAffinity getEspWifiAffinity() {
#ifdef CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_0
    return 0;
#elif defined(CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1)
    return 1;
#endif
}

// Warning: Must watch ESP WiFi, as this task is used by WiFi
static CpuAffinity getEspMainSchedulerAffinity() {
#ifdef CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_0
    return 0;
#elif defined(CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1)
    return 1;
#endif
}

static CpuAffinity getFreeRtosTimerAffinity() {
#if defined(CONFIG_FREERTOS_TIMER_TASK_NO_AFFINITY)
    return None;
#elif defined(CONFIG_FREERTOS_TIMER_TASK_AFFINITY_CPU0)
    return 0;
#elif defined(CONFIG_FREERTOS_TIMER_TASK_AFFINITY_CPU1)
    return 1;
#else
    static_assert(false);
#endif
}

#if CONFIG_FREERTOS_NUMBER_OF_CORES == 1
static const CpuAffinityConfiguration esp = {
    .system = 0,
    .graphics = 0,
    .wifi = 0,
    .mainDispatcher = 0,
    .apps = 0,
    .timer = getFreeRtosTimerAffinity()
};
#elif CONFIG_FREERTOS_NUMBER_OF_CORES == 2
static const CpuAffinityConfiguration esp = {
    .system = 0,
    .graphics = 1,
    .wifi = getEspWifiAffinity(),
    .mainDispatcher = getEspMainSchedulerAffinity(),
    .apps = 1,
    .timer = getFreeRtosTimerAffinity()
};
#endif

#else

static const CpuAffinityConfiguration simulator = {
    .system = None,
    .graphics = None,
    .wifi = None,
    .mainDispatcher = 0,
    .apps = None,
    .timer = None
};

#endif

const CpuAffinityConfiguration& getCpuAffinityConfiguration() {
#ifdef ESP_PLATFORM

#if CONFIG_FREERTOS_NUMBER_OF_CORES == 2
    // WiFi uses the main dispatcher to defer operations in the background
    assert(esp.wifi == esp.mainDispatcher);
#endif // CORES
    return esp;

#else
    return simulator;
#endif
}

}
