#include "nb_display.h"
#include "nb_internal.h"
#include <esp_check.h>
#include <esp_log.h>

esp_err_t nb_display_create(nb_display_driver_t driver, nb_display_t* display) {
    ESP_RETURN_ON_ERROR(driver.create_display(display), nbi_tag, "failed to create driver");
    return ESP_OK;
}
