#include "hello_world/hello_world.h"
#include "lvgl_hal.h"
#include "tactility.h"
#include "ui/lvgl_sync.h"

#include "FreeRTOS.h"
#include "task.h"

#define TAG "main"

extern HardwareConfig sim_hardware;

_Noreturn void app_main() {
    static const Config config = {
        .hardware = &sim_hardware,
        .apps = {
            &hello_world_app
        },
        .services = {},
        .auto_start_app_id = NULL
    };

    TT_LOG_I("app", "Hello, world!");

    tt_init(&config);

    while (true) {
        vTaskDelay(1000);
    }
}
