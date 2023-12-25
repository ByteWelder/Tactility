#include "nb_platform.h"
#include <esp_check.h>
#include "nb_display.h"
#include "nb_touch.h"
#include "nb_internal.h"

static const char* TAG = "nb_platform";

esp_err_t nb_platform_create(nb_platform_config_t config, nb_platform_t* platform) {
    ESP_LOGI(TAG, "display with driver %s", config.display_driver.name);
    ESP_RETURN_ON_ERROR(
        nb_display_create(config.display_driver, &(platform->display)),
        nbi_tag,
        "display driver init failed"
    );

    ESP_LOGI(TAG, "touch with driver %s", config.touch_driver.name);
    ESP_RETURN_ON_ERROR(
        nb_touch_create(config.touch_driver, &(platform->touch)),
        nbi_tag,
        "touch driver init failed"
    );

    return ESP_OK;
}
