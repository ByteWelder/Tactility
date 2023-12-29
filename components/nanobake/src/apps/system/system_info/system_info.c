#include "system_info.h"
#include "furi_extra_defines.h"
#include "nanobake.h"
#include "thread.h"

static int32_t system_info_entry_point(void* param) {
    UNUSED(param);

    // Wait for all apps to start
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf(
        "Heap memory available: %d / %d\n",
        heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
        heap_caps_get_total_size(MALLOC_CAP_DEFAULT)
    );

    return 0;
}

AppManifest system_info_app = {
    .id = "systeminfo",
    .name = "System Info",
    .icon = NULL,
    .type = AppTypeSystem,
    .entry_point = &system_info_entry_point,
    .stack_size = AppStackSizeNormal
};
