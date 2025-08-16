#include "Ft6x36Touch.h"

#include <Ft6x36Touch.h>
#include <Tactility/Log.h>

#include <esp_err.h>
#include <esp_lvgl_port.h>

#define TAG "ft6x36"

void Ft6x36Touch::touchReadCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    auto* touch = (Ft6x36Touch*)lv_indev_get_driver_data(indev);
    touch->mutex.lock();
    data->point = touch->lastPoint;
    data->state = touch->lastState;
    touch->mutex.unlock();
}

Ft6x36Touch::Ft6x36Touch(std::unique_ptr<Configuration> inConfiguration) :
    configuration(std::move(inConfiguration)) {
    nativeTouch = std::make_shared<Ft6TouchDriver>(*this);
}

Ft6x36Touch::~Ft6x36Touch() {
    if (driverThread != nullptr && driverThread->getState() != tt::Thread::State::Stopped) {
        interruptDriverThread = true;
        driverThread->join();
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

bool Ft6x36Touch::shouldInterruptDriverThread() const {
    bool interrupt = false;
    if (mutex.lock(50 / portTICK_PERIOD_MS)) {
        interrupt = interruptDriverThread;
        mutex.unlock();
    }
    return interrupt;
}

bool Ft6x36Touch::start() {
    TT_LOG_I(TAG, "Start");

    if (!driver.begin(FT6X36_DEFAULT_THRESHOLD, configuration->width, configuration->height)) {
        TT_LOG_E(TAG, "driver.begin() failed");
        return false;
    }

    mutex.lock();

    interruptDriverThread = false;

    driverThread = std::make_shared<tt::Thread>("ft6x36", 4096, [this] {
        driverThreadMain();
        return 0;
    });

    driverThread->start();

    mutex.unlock();

    return true;
}

bool Ft6x36Touch::stop() {
    TT_LOG_I(TAG, "Stop");

    mutex.lock();
    interruptDriverThread = true;
    mutex.unlock();

    driverThread->join();

    mutex.lock();
    driverThread = nullptr;
    mutex.unlock();

    return false;
}

bool Ft6x36Touch::startLvgl(lv_display_t* display) {
    if (deviceHandle != nullptr) {
        return false;
    }

    deviceHandle = lv_indev_create();
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_POINTER);
    lv_indev_set_driver_data(deviceHandle, this);
    lv_indev_set_read_cb(deviceHandle, touchReadCallback);

    return true;
}

bool Ft6x36Touch::stopLvgl() {
    if (deviceHandle == nullptr) {
        return false;
    }

    lv_indev_delete(deviceHandle);
    deviceHandle = nullptr;
    return true;
}
