#include "nb_touch.h"
#include <esp_check.h>
#include "nb_internal.h"

esp_err_t nb_touch_create(nb_touch_driver_t driver, nb_touch_t* touch) {
    ESP_RETURN_ON_ERROR(driver.init_io(&(touch->io_handle)), nbi_tag, "failed to init io");
    ESP_RETURN_ON_ERROR(driver.create_touch(touch->io_handle, &(touch->touch_handle)), nbi_tag, "failed to create driver");
    return ESP_OK;
}
