#include "Check.h"

#include "Log.h"
#include "RtosCompatTask.h"

#define TAG "kernel"

static void logMemoryInfo() {
#ifdef ESP_PLATFORM
    TT_LOG_E(TAG, "default caps:");
    TT_LOG_E(TAG, "  total: %u", heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
    TT_LOG_E(TAG, "  free: %u", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    TT_LOG_E(TAG, "  min free: %u", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    TT_LOG_E(TAG, "internal caps:");
    TT_LOG_E(TAG, "  total: %u", heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
    TT_LOG_E(TAG, "  free: %u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    TT_LOG_E(TAG, "  min free: %u", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
#endif
}

static void logTaskInfo() {
    const char* name = pcTaskGetName(nullptr);
    const char* safe_name = name ? name : "main";
    TT_LOG_E(TAG, "Task: %s", safe_name);
    TT_LOG_E(TAG, "Stack watermark: %u", uxTaskGetStackHighWaterMark(NULL) * 4);
}

namespace tt {

TT_NORETURN void _crash() {
    logTaskInfo();
    logMemoryInfo();
    // TODO: Add breakpoint when debugger is attached.
#ifdef ESP_TARGET
    esp_system_abort("System halted. Connect debugger for more info.");
#endif
    __builtin_unreachable();
}

}
