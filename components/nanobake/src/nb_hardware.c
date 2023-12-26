#include "nb_hardwarei.h"
#include <esp_check.h>
#include <esp_err.h>
#include <check.h>

static const char* TAG = "nb_hardware";

nb_hardware_t nb_hardware_create(nb_config_t _Nonnull* config) {

    furi_check(config->display_driver != NULL, "no display driver configured");
    nb_display_driver_t display_driver = config->display_driver();
    ESP_LOGI(TAG, "display with driver %s", display_driver.name);
    nb_display_t* display = nb_display_alloc(&display_driver);

    nb_touch_t* touch = NULL;
    if (config->touch_driver != NULL) {
        nb_touch_driver_t touch_driver = config->touch_driver();
        ESP_LOGI(TAG, "touch with driver %s", touch_driver.name);
        touch = nb_touch_alloc(&touch_driver);
    } else {
        ESP_LOGI(TAG, "no touch configured");
        touch = NULL;
    }

    return (nb_hardware_t) {
        .display = display,
        .touch = touch
    };
}
