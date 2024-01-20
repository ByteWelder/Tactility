#include "check.h"
#include "hardware_i.h"
#include "touch.h"

#define TAG "hardware"

Hardware tt_hardware_init(const HardwareConfig* config) {
    if (config->bootstrap != NULL) {
        TT_LOG_I(TAG, "Bootstrapping");
        config->bootstrap();
    }

    tt_check(config->display_driver != NULL, "no display driver configured");
    DisplayDriver display_driver = config->display_driver();
    TT_LOG_I(TAG, "display with driver %s", display_driver.name);
    DisplayDevice* display = tt_display_device_alloc(&display_driver);

    TouchDevice* touch = NULL;
    if (config->touch_driver != NULL) {
        TouchDriver touch_driver = config->touch_driver();
        TT_LOG_I(TAG, "touch with driver %s", touch_driver.name);
        touch = tt_touch_alloc(&touch_driver);
    } else {
        TT_LOG_I(TAG, "no touch configured");
        touch = NULL;
    }

    return (Hardware) {
        .display = display,
        .touch = touch
    };
}
