#include "hello_world/hello_world.h"
#include "lvgl_hal.h"
#include "tactility.h"
#include "ui/lvgl_sync.h"

#include "FreeRTOS.h"
#include "task.h"

#define TAG "main"

void lvgl_task(TT_UNUSED void* parameter);

_Noreturn void app_main(TT_UNUSED void* parameters) {
    static const Config config = {
        .apps = {
            &hello_world_app
        },
        .services = {},
        .auto_start_app_id = NULL
    };

    TT_LOG_I("app", "Hello, world!");

    tt_lvgl_sync_set(&lvgl_lock, &lvgl_unlock);
//    tt_init(&config);

    while (true) {
        vTaskDelay(1000);
    }
}
