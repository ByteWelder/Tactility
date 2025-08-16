#include "Drivers.h"

#include <esp_log.h>

constexpr auto TAG = "Drivers";

bool Drivers::validateSupport() {
    uint16_t display_count = 0;
    if (!tt_hal_device_find(DEVICE_TYPE_DISPLAY, &displayId, &display_count, 1)) {
        ESP_LOGI(TAG, "No display device found");
        return false;
    }

    if (!tt_hal_display_driver_supported(displayId)) {
        ESP_LOGI(TAG, "Display doesn't support driver interface");
        return false;
    }

    uint16_t touch_count = 0;
    if (!tt_hal_device_find(DEVICE_TYPE_TOUCH, &touchId, &touch_count, 1)) {
        ESP_LOGI(TAG, "No touch device found");
        return false;
    }

    if (!tt_hal_touch_driver_supported(touchId)) {
        ESP_LOGI(TAG, "Touch doesn't support driver interface");
        return false;
    }

    return true;
}

bool Drivers::start() {
    display = tt_hal_display_driver_alloc(displayId);
    touch = tt_hal_touch_driver_alloc(touchId);
    return true;
}

void Drivers::stop() {
    tt_hal_display_driver_free(display);
    tt_hal_touch_driver_free(touch);
    display = nullptr;
    touch = nullptr;
}
