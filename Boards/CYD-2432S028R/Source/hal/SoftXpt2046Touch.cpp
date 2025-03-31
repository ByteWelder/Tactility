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
    if (!touch.begin()) {
        TT_LOG_E(TAG, "Failed to initialize XPT2046 soft SPI");
        return false;
    }
    touch.setRotation(1);  // Match display orientation

    // LVGL v9
    indev = lv_indev_create();
    if (indev == nullptr) {
        TT_LOG_E(TAG, "Failed to create LVGL input device");
        return false;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, readCallback);
    lv_indev_set_user_data(indev, this);

    ESP_LOGI(TAG, "Software SPI touch initialized");
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

    if (z >= Z_THRESHOLD) {
        int16_t tx = x, ty = y;
        if (touch->config->swapXy) std::swap(tx, ty);
        if (touch->config->mirrorX) tx = 4095 - tx;
        if (touch->config->mirrorY) ty = 4095 - ty;
        data->point.x = (tx * touch->config->xMax) / 4095;
        data->point.y = (ty * touch->config->yMax) / 4095;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
