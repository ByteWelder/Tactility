#include "M5stackTouch.h"
#include "M5Unified.hpp"
#include "esp_err.h"
#include "Log.h"

#define TAG "m5stack_touch"

static void touchReadCallback(TT_UNUSED lv_indev_t* indev, lv_indev_data_t* data) {
    lgfx::touch_point_t point; // Making it static makes it unreliable
    bool touched = M5.Lcd.getTouch(&point) > 0;
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = point.x;
        data->point.y = point.y;
    }
}

_Nullable lv_indev_t* createTouch() {
    static lv_indev_t* indev = lv_indev_create();
    LV_ASSERT_MALLOC(indev)
    if (indev == nullptr) {
        return nullptr;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchReadCallback);
    return indev;
}
bool M5stackTouch::start(lv_display_t* display) {

    TT_LOG_I(TAG, "Adding touch to LVGL");
    deviceHandle = createTouch();
    if (deviceHandle == nullptr) {
        TT_LOG_E(TAG, "Adding touch failed");
        return false;
    }

    return true;
}

bool M5stackTouch::stop() {
    tt_assert(deviceHandle != nullptr);
    lv_indev_delete(deviceHandle);
    deviceHandle = nullptr;
    return true;
}
