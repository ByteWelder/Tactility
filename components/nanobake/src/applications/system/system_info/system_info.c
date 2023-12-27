#include "system_info.h"
#include "nanobake.h"
#include "core_defines.h"
#include "thread.h"

static int32_t system_info_entry_point(void* param) {
    UNUSED(param);

    // Wait for all apps to start
    vTaskDelay(1000  / portTICK_PERIOD_MS);

    size_t system_service_count = nanobake_get_app_thread_count();
    printf("Running apps:\n");
    for (int i = 0; i < system_service_count; ++i) {
        FuriThreadId thread_id = nanobake_get_app_thread_id(i);
        const char* appid = furi_thread_get_appid(thread_id);
        const char* name = furi_thread_get_name(thread_id);
        bool is_suspended = furi_thread_is_suspended(thread_id);
        const char* status = is_suspended ? "suspended" : "active";
        printf(" - [%s] %s (%s)\n", status, name, appid);
    }

    printf("Heap memory available: %d / %d\n",
        heap_caps_get_free_size(MALLOC_CAP_DEFAULT),
        heap_caps_get_total_size(MALLOC_CAP_DEFAULT)
    );

    return 0;
}

NbApp system_info_app = {
    .id = "systeminfo",
    .name = "System Info",
    .type = SYSTEM,
    .entry_point = &system_info_entry_point,
    .stack_size = NB_TASK_STACK_SIZE_DEFAULT,
    .priority = NB_TASK_PRIORITY_DEFAULT
};
