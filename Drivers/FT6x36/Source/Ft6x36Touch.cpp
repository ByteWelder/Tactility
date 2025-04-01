#include "Ft6x36Touch.h"

#include <Tactility/Log.h>

#include <esp_err.h>
#include <esp_lvgl_port.h>

#define TAG "ft6x36"

static void touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* touch = (Ft6x36Touch*)lv_indev_get_driver_data(indev);
    touch->readLast(data);
}

Ft6x36Touch::Ft6x36Touch(std::unique_ptr<Configuration> inConfiguration) :
    configuration(std::move(inConfiguration)),
    driverThread(tt::Thread("ft6x36", 4096, [this]() {
        driverThreadMain();
        return 0;
    }))
{}

Ft6x36Touch::~Ft6x36Touch() {
    if (driverThread.getState() != tt::Thread::State::Stopped) {
        stop();
    }
}

void Ft6x36Touch::driverThreadMain() {
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

bool Ft6x36Touch::shouldInterruptDriverThread() {
    bool interrupt = false;
    if (mutex.lock(50 / portTICK_PERIOD_MS)) {
        interrupt = interruptDriverThread;
        mutex.unlock();
    }
    return interrupt;
}

bool Ft6x36Touch::start(lv_display_t* display) {
    TT_LOG_I(TAG, "start");

    driverThread.start();

    uint16_t width = lv_display_get_horizontal_resolution(display);
    uint16_t height = lv_display_get_vertical_resolution(display);
    if (!driver.begin(FT6X36_DEFAULT_THRESHOLD, width, height)) {
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

bool Ft6x36Touch::stop() {
    lv_indev_delete(deviceHandle);
    interruptDriverThread = true;
    driverThread.join();
    return true;
}

void Ft6x36Touch::readLast(lv_indev_data_t* data) {
    data->point = lastPoint;
    data->state = lastState;
}
