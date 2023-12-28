#include "check.h"
#include "devices_i.h"
#include "esp_check.h"
#include "esp_err.h"

#define TAG "hardware"

Devices nb_devices_create(Config _Nonnull* config) {
    furi_check(config->display_driver != NULL, "no display driver configured");
    DisplayDriver display_driver = config->display_driver();
    ESP_LOGI(TAG, "display with driver %s", display_driver.name);
    DisplayDevice* display = nb_display_alloc(&display_driver);

    TouchDevice* touch = NULL;
    if (config->touch_driver != NULL) {
        TouchDriver touch_driver = config->touch_driver();
        ESP_LOGI(TAG, "touch with driver %s", touch_driver.name);
        touch = nb_touch_alloc(&touch_driver);
    } else {
        ESP_LOGI(TAG, "no touch configured");
        touch = NULL;
    }

    return (Devices) {
        .display = display,
        .touch = touch
    };
}
