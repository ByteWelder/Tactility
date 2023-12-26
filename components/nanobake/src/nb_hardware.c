#include "nb_hardware.h"
#include "nb_display.h"
#include "nb_touch.h"

#include <esp_check.h>
#include <esp_err.h>
#include <esp_lvgl_port.h>

#include <check.h>

static const char* TAG = "nb_hardware";

nb_hardware_t _Nonnull* nb_hardware_alloc(nb_config_t _Nonnull* config) {
    nb_hardware_t* platform = malloc(sizeof(nb_hardware_t));

    furi_check(config->display_driver != NULL, "no display driver configured");
    nb_display_driver_t display_driver = config->display_driver();
    ESP_LOGI(TAG, "display with driver %s", display_driver.name);
    platform->display = nb_display_alloc(&display_driver);

    if (config->touch_driver != NULL) {
        nb_touch_driver_t touch_driver = config->touch_driver();
        ESP_LOGI(TAG, "touch with driver %s", touch_driver.name);
        platform->touch = nb_touch_alloc(&touch_driver);
    } else {
        ESP_LOGI(TAG, "no touch configured");
        platform->touch = NULL;
    }


    return platform;
}
