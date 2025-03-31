#include "SoftXpt2046Touch.h"
#include <Tactility/Log.h>
#include <Tactility/lvgl/LvglSync.h>
#include "esp_log.h"
#include <inttypes.h>

#define TAG "soft_xpt2046"

SoftXpt2046Touch::SoftXpt2046Touch(std::unique_ptr<Configuration> config)
    : config(std::move(config)), touch(CYD_TOUCH_CS_PIN, CYD_TOUCH_IRQ_PIN) {
    assert(this->config != nullptr);
}

bool SoftXpt2046Touch::start(lv_display_t* display) {
    ESP_LOGI(TAG, "Initializing software SPI touch");

    if (!touch.begin()) {
        TT_LOG_E(TAG, "Failed to initialize XPT2046 soft SPI");
        return false;
    }
    ESP_LOGI(TAG, "XPT2046 soft SPI initialized successfully");

    touch.setRotation(1);  // Try 0, 1, 2, 3 to match display
    ESP_LOGI(TAG, "Touch rotation set to 1");

    // Comment out calibration for now to test raw coordinates
    // if (config->xfac != 0 || config->yfac != 0 || config->xoff != 0 || config->yoff != 0) {
    //     touch.calibrate(config->xfac, config->yfac, config->xoff, config->yoff);
    // }

    indev = lv_indev_create();
    if (indev == nullptr) {
        TT_LOG_E(TAG, "Failed to create LVGL input device");
        return false;
    }
    ESP_LOGI(TAG, "LVGL input device created at %p", indev);

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, readCallback);
    lv_indev_set_user_data(indev, this);

    ESP_LOGI(TAG, "Software SPI touch initialized successfully with xMax=%" PRIu16 ", yMax=%" PRIu16 ", swapXy=%d, mirrorX=%d, mirrorY=%d",
             config->xMax, config->yMax, config->swapXy, config->mirrorX, config->mirrorY);
    return true;
}

bool SoftXpt2046Touch::stop() {
    if (indev != nullptr) {
        ESP_LOGI(TAG, "Deleting LVGL input device at %p", indev);
        lv_indev_delete(indev);
        indev = nullptr;
    }
    ESP_LOGI(TAG, "Software SPI touch stopped");
    return true;
}

void SoftXpt2046Touch::readCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* touch = static_cast<SoftXpt2046Touch*>(lv_indev_get_user_data(indev));
    uint16_t x, y, z;
    touch->touch.readData(&x, &y, &z);

    ESP_LOGI(TAG, "Touch raw: x=%" PRIu16 ", y=%" PRIu16 ", z=%" PRIu16, x, y, z);

    if (z > 0) {
        int16_t tx = x, ty = y;
        if (touch->config->swapXy) std::swap(tx, ty);
        if (touch->config->mirrorX) tx = touch->config->xMax - tx;
        if (touch->config->mirrorY) ty = touch->config->yMax - ty;
        data->point.x = tx;
        data->point.y = ty;
        data->state = LV_INDEV_STATE_PRESSED;
        ESP_LOGI(TAG, "Touch mapped: x=%" PRId32 ", y=%" PRId32, data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
