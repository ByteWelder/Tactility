#include "Core2Touch.h"
#include <esp_lvgl_port.h>

#define TAG "core2_touch"

static void touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* touch = (Core2Touch*)lv_indev_get_driver_data(indev);
    touch->readLast(data);
}

static int32_t threadCallback(void* context) {
    auto* touch = (Core2Touch*)context;
    touch->driverThreadMain();
    return 0;
}

Core2Touch::Core2Touch() :
    driverThread(tt::Thread("", 4096, threadCallback, this))
{ }

void Core2Touch::driverThreadMain() {
    TPoint point = { .x = 0, .y = 0 };
    TEvent event = TEvent::None;

    while (!shouldInterruptDriverThread()) {
        driver.processTouch();
        driver.poll(&point, &event);

        if (mutex.lock(100)) {
            switch (event) {
                case TEvent::TouchStart:
                case TEvent::TouchMove:
                case TEvent::DragStart:
                case TEvent::DragMove:
                case TEvent::DragEnd:
                    lastState = LV_INDEV_STATE_PR;
                    lastPoint.x = point.x;
                    lastPoint.y = point.y;
                    break;
                case TEvent::TouchEnd:
                    lastState = LV_INDEV_STATE_REL;
                    lastPoint.x = point.x;
                    lastPoint.y = point.y;
                    break;
                case TEvent::Tap:
                case TEvent::None:
                    break;
            }
            mutex.unlock();
        }
    }
}

bool Core2Touch::shouldInterruptDriverThread() {
    bool interrupt = false;
    if (mutex.lock(50 / portTICK_PERIOD_MS)) {
        interrupt = interruptDriverThread;
        mutex.unlock();
    }
    return interrupt;
}

bool Core2Touch::start(lv_display_t* display) {
    TT_LOG_I(TAG, "start");

    driverThread.start();

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
    interruptDriverThread = true;
    driverThread.join();
    return true;
}

void Core2Touch::readLast(lv_indev_data_t* data) {
    data->point = lastPoint;
    data->state = lastState;
}
