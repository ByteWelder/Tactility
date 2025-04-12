#include "SoftXpt2046Touch.h"
#include "esp_log.h"
#include <inttypes.h>  // For PRId32

static const char* TAG = "soft_xpt2046";

SoftXpt2046Touch::SoftXpt2046Touch(std::unique_ptr<Configuration> configuration)
    : configuration(std::move(configuration)),
      touch(CYD_TOUCH_CS_PIN, CYD_TOUCH_IRQ_PIN) {}

bool SoftXpt2046Touch::init() {
    touch.begin();
    touch.setRotation(0);
    ESP_LOGI(TAG, "Software SPI touch initialized with xMax=%u, yMax=%u, swapXy=%d, mirrorX=%d, mirrorY=%d",
             configuration->xMax, configuration->yMax, configuration->swapXy,
             configuration->mirrorX, configuration->mirrorY);
    return true;
}

bool SoftXpt2046Touch::start(lv_display_t* display) {
    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, [](lv_indev_t* indev, lv_indev_data_t* data) {
        auto* touch = static_cast<SoftXpt2046Touch*>(lv_indev_get_user_data(indev));
        touch->read(data);
    });
    lv_indev_set_user_data(indev, this);
    return true;
}

bool SoftXpt2046Touch::stop() {
    if (indev) {
        lv_indev_delete(indev);
        indev = nullptr;
    }
    return true;
}

bool SoftXpt2046Touch::read(lv_indev_data_t* data) {
    if (!data) return false;

    TS_Point point = touch.getPoint();
    if (point.z == 0) {
        data->state = LV_INDEV_STATE_REL;
        return false;
    }

    int32_t x = point.x;
    int32_t y = point.y;

    // Log raw values
    ESP_LOGI(TAG, "Touch raw (rotated): x=%" PRId32 ", y=%" PRId32 ", z=%" PRId32, x, y, point.z);

    // Scale to screen coordinates
    x = (x - configuration->xMinRaw) * configuration->xMax / (configuration->xMaxRaw - configuration->xMinRaw);
    y = (y - configuration->yMinRaw) * configuration->yMax / (configuration->yMaxRaw - configuration->yMinRaw);

    ESP_LOGI(TAG, "Post-X calc: tx=%" PRId32, x);
    ESP_LOGI(TAG, "Post-Y calc: ty=%" PRId32, y);

    // Clamp to screen bounds
    x = x < 0 ? 0 : (x > configuration->xMax ? configuration->xMax : x);
    y = y < 0 ? 0 : (y > configuration->yMax ? configuration->yMax : y);

    ESP_LOGI(TAG, "Pre-mapped: tx=%" PRId32 ", ty=%" PRId32, x, y);

    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PR;

    ESP_LOGI(TAG, "Touch mapped: x=%" PRId32 ", y=%" PRId32, data->point.x, data->point.y);
    return true;
}
