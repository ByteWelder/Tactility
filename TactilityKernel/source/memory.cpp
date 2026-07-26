#include <tactility/log.h>
#include <tactility/memory.h>

#ifdef ESP_PLATFORM
#include <esp_heap_caps.h>
#endif

constexpr auto* TAG = "memory";

extern "C" {

void memory_trace() {
#ifdef ESP_PLATFORM
    size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t heap_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    LOG_I(TAG, "Heap: %zu / %zu available", heap_free, heap_total);
    size_t ext_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t ext_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    LOG_I(TAG, "External: %zu / %zu available", ext_free, ext_total);
#endif
}

}
