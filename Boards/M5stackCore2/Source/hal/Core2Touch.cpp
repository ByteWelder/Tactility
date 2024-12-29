#include "Core2Touch.h"
#include <esp_lvgl_port.h>

#define TAG "core2_touch"

static void touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* touch = (Core2Touch*)lv_indev_get_driver_data(indev);
    touch->read(data);
}

bool Core2Touch::start(lv_display_t* display) {
    TT_LOG_I(TAG, "start");
    uint16_t width = lv_display_get_horizontal_resolution(display);
    uint16_t height = lv_display_get_vertical_resolution(display);
    if (!driver.begin(5, width, height)) {
        TT_LOG_E(TAG, "driver.begin() failed");
        return false;
    }

    deviceHandle = lv_indev_create();
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_POINTER);
    lv_indev_set_driver_data(deviceHandle, this);
    lv_indev_set_read_cb(deviceHandle, touchReadCallback);

    TT_LOG_I(TAG, "start success");
    return true;
}

bool Core2Touch::stop() {
    lv_indev_delete(deviceHandle);
    return true;
}

void Core2Touch::read(lv_indev_data_t* data) {
    TPoint point;
    TEvent event;
    driver.processTouch();
    driver.poll(&point, &event);

    switch (event) {
        case TEvent::TouchStart:
        case TEvent::TouchMove:
        case TEvent::DragStart:
        case TEvent::DragMove:
        case TEvent::DragEnd:
            data->state = LV_INDEV_STATE_PR;
            data->point.x = point.x;
            data->point.y = point.y;
            break;
        case TEvent::TouchEnd:
            data->state = LV_INDEV_STATE_REL;
            data->point.x = point.x;
            data->point.y = point.y;
            break;
        case TEvent::Tap:
        case TEvent::None:
            break;
    }
}