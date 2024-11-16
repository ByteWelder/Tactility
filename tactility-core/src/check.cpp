#include "check.h"

#include "core_defines.h"
#include "log.h"

#ifdef ESP_TARGET
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "task.h"
#endif

#define TAG "kernel"

static void tt_print_memory_info() {
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

static void tt_print_task_info() {
    const char* name = pcTaskGetName(nullptr);
    const char* safe_name = name ? name : "main";
    TT_LOG_E(TAG, "Task: %s", safe_name);
    TT_LOG_E(TAG, "Stack watermark: %u", uxTaskGetStackHighWaterMark(NULL) * 4);
}

TT_NORETURN void tt_crash_implementation() {
    tt_print_task_info();
    tt_print_memory_info();
    // TODO: Add breakpoint when debugger is attached.
#ifdef ESP_TARGET
    esp_system_abort("System halted. Connect debugger for more info.");
#endif
    __builtin_unreachable();
}
