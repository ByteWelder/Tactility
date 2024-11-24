#include "M5Unified.hpp"
#include "lvgl.h"
#include "TactilityCore.h"

#define TAG "cores3_touch"

static void read_callback(TT_UNUSED lv_indev_t* indev, lv_indev_data_t* data) {
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

_Nullable lv_indev_t* m5stack_lvgl_touch() {
    static lv_indev_t* indev = lv_indev_create();
    LV_ASSERT_MALLOC(indev)
    if (indev == nullptr) {
        return nullptr;
    }

    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, read_callback);
    return indev;
}
