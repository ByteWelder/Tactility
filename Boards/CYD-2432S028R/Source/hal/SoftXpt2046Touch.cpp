#include "SoftXpt2046Touch.h"
#include <Tactility/Log.h>
#include <Tactility/lvgl/LvglSync.h>
#include "esp_log.h"

#define TAG "soft_xpt2046"

SoftXpt2046Touch::SoftXpt2046Touch(std::unique_ptr<Configuration> config)
    : config(std::move(config)), touch(CYD_TOUCH_CS_PIN, CYD_TOUCH_IRQ_PIN) {
    assert(this->config != nullptr);
}

bool SoftXpt2046Touch::start(lv_display_t* display) {
    ESP_LOGI(TAG, "Initializing software SPI touch");

    // Check if touch initialization succeeds
    if (!touch.begin()) {
        TT_LOG_E(TAG, "Failed to initialize XPT2046 soft SPI");
        return false;
    }
    ESP_LOGI(TAG, "XPT2046 soft SPI initialized successfully");
    touch.setRotation(1);  // Match display orientation
    ESP_LOGI(TAG, "Touch rotation set to 1");

    // LVGL v9: Create input device
    indev = lv_indev_create();
    if (indev == nullptr) {
        TT_LOG_E(TAG, "Failed to create LVGL input device");
        return false;
    }
    ESP_LOGI(TAG, "LVGL input device created at %p", indev);

    // Set input device type
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    ESP_LOGI(TAG, "Input device type set to POINTER");

    // Set read callback
    lv_indev_set_read_cb(indev, readCallback);
    ESP_LOGI(TAG, "Read callback set to SoftXpt2046Touch::readCallback");

    // Set user data
    lv_indev_set_user_data(indev, this);
    ESP_LOGI(TAG, "User data set to this=%p", this);

    ESP_LOGI(TAG, "Software SPI touch initialized successfully");
    return true;
}

bool SoftXpt2046Touch::stop() {
    if (indev != nullptr) {
        lv_indev_delete(indev);
        indev = nullptr;
    }
    return true;
}

void SoftXpt2046Touch::readCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* touch = static_cast<SoftXpt2046Touch*>(lv_indev_get_user_data(indev));
    uint16_t x, y, z;
    touch->touch.readData(&x, &y, &z);

    ESP_LOGI(TAG, "Touch raw: x=%d, y=%d, z=%d", x, y, z);  // Add logging

    if (z >= Z_THRESHOLD) {
        int16_t tx = x, ty = y;
        if (touch->config->swapXy) std::swap(tx, ty);
        if (touch->config->mirrorX) tx = 4095 - tx;
        if (touch->config->mirrorY) ty = 4095 - ty;
        data->point.x = (tx * touch->config->xMax) / 4095;
        data->point.y = (ty * touch->config->yMax) / 4095;
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "Touch mapped: x=%d, y=%d", data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
