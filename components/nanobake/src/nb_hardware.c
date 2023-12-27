#include "nb_hardware_i.h"
#include "esp_check.h"
#include "esp_err.h"
#include "check.h"

#define TAG "hardware"

NbHardware nb_hardware_create(NbConfig _Nonnull* config) {

    furi_check(config->display_driver != NULL, "no display driver configured");
    NbDisplayDriver display_driver = config->display_driver();
    ESP_LOGI(TAG, "display with driver %s", display_driver.name);
    NbDisplay* display = nb_display_alloc(&display_driver);

    NbTouch* touch = NULL;
    if (config->touch_driver != NULL) {
        NbTouchDriver touch_driver = config->touch_driver();
        ESP_LOGI(TAG, "touch with driver %s", touch_driver.name);
        touch = nb_touch_alloc(&touch_driver);
    } else {
        ESP_LOGI(TAG, "no touch configured");
        touch = NULL;
    }

    return (NbHardware) {
        .display = display,
        .touch = touch
    };
}
