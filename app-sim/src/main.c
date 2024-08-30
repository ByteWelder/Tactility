#include "hello_world/hello_world.h"
#include "tactility.h"
#include "assets.h"

#include "FreeRTOS.h"
#include "ui/statusbar.h"

#define TAG "main"

extern HardwareConfig sim_hardware;

void app_main() {
    static const Config config = {
        .hardware = &sim_hardware,
        .apps = {
            &hello_world_app
        },
        .services = {},
        .auto_start_app_id = NULL
    };

    tt_init(&config);

    // Mock icon for screenshots
    tt_statusbar_icon_add(TT_ASSETS_ICON_SDCARD_ALERT);
}
