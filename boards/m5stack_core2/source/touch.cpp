#include "tactility_core.h"
#include "lvgl.h"
#include "M5Unified.hpp"

#define TAG "core2_touch"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Touch seems to be offset by a certain amount.
 * The docs don't mention it, so this is the estimated value.
 */
#define TOUCH_Y_OFFSET 16

static void read_touch(TT_UNUSED lv_indev_t* indev, lv_indev_data_t* data) {
    lgfx::touch_point_t point; // Making it static makes it unreliable
    bool touched = M5.Lcd.getTouch(&point) > 0;
    if (touched) {
        data->point.x = point.x;
        data->point.y = point.y - TOUCH_Y_OFFSET;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool core2_touch_init() {
    lv_indev_t _Nullable* touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, read_touch);
    return true;
}

#ifdef __cplusplus
}
#endif
