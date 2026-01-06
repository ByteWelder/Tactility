#include <Tactility/Check.h>

#include <Tactility/Logger.h>
#include <Tactility/freertoscompat/Task.h>

static const auto LOGGER = tt::Logger("kernel");

static void logMemoryInfo() {
#ifdef ESP_PLATFORM
    LOGGER.error("default caps:");
    LOGGER.error("  total: {}", heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
    LOGGER.error("  free: {}", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    LOGGER.error("  min free: {}", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    LOGGER.error("internal caps:");
    LOGGER.error("  total: {}", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
    LOGGER.error("  free: {}", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    LOGGER.error("  min free: {}", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
#endif
}

static void logTaskInfo() {
    const char* name = pcTaskGetName(nullptr);
    const char* safe_name = name ? name : "main";
    LOGGER.error("Task: {}", safe_name);
    LOGGER.error("Stack watermark: {}", uxTaskGetStackHighWaterMark(NULL) * 4);
}

namespace tt {

TT_NORETURN void _crash() {
    logTaskInfo();
    logMemoryInfo();
    // TODO: Add breakpoint when debugger is attached.
#ifdef ESP_PLATFORM
    esp_system_abort("System halted. Connect debugger for more info.");
#endif
    __builtin_unreachable();
}

}
