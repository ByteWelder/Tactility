#include "check.h"
#include "devices_i.h"
#include "esp_check.h"
#include "esp_err.h"

#define TAG "hardware"

Hardware tt_hardware_init(const HardwareConfig _Nonnull* config) {
    if (config->bootstrap != NULL) {
        ESP_LOGI(TAG, "Bootstrapping");
        config->bootstrap();
    }

    tt_check(config->display_driver != NULL, "no display driver configured");
    DisplayDriver display_driver = config->display_driver();
    ESP_LOGI(TAG, "display with driver %s", display_driver.name);
    DisplayDevice* display = tt_display_device_alloc(&display_driver);

    TouchDevice* touch = NULL;
    if (config->touch_driver != NULL) {
        TouchDriver touch_driver = config->touch_driver();
        ESP_LOGI(TAG, "touch with driver %s", touch_driver.name);
        touch = tt_touch_alloc(&touch_driver);
    } else {
        ESP_LOGI(TAG, "no touch configured");
        touch = NULL;
    }

    return (Hardware) {
        .display = display,
        .touch = touch
    };
}
