#include "ButtonControl.h"

#include <Tactility/Log.h>

#include <esp_lvgl_port.h>

constexpr auto* TAG = "ButtonControl";

ButtonControl::ButtonControl(const std::vector<PinConfiguration>& pinConfigurations) : pinConfigurations(pinConfigurations) {
    pinStates.resize(pinConfigurations.size());
    for (const auto& pinConfiguration : pinConfigurations) {
        tt::hal::gpio::configure(pinConfiguration.pin, tt::hal::gpio::Mode::Input, false, false);
    }
}

ButtonControl::~ButtonControl() {
    if (driverThread != nullptr && driverThread->getState() != tt::Thread::State::Stopped) {
        interruptDriverThread = true;
        driverThread->join();
    }
}

void ButtonControl::readCallback(lv_indev_t* indev, lv_indev_data_t* data) {
    // Defaults
    data->enc_diff = 0;
    data->state = LV_INDEV_STATE_RELEASED;

    auto* self = static_cast<ButtonControl*>(lv_indev_get_driver_data(indev));

    if (self->mutex.lock(100)) {

        for (int i = 0; i < self->pinConfigurations.size(); i++) {
            const auto& config = self->pinConfigurations[i];
            std::vector<PinState>::reference state = self->pinStates[i];
            const bool trigger = (config.event == Event::ShortPress && state.triggerShortPress) ||
                (config.event == Event::LongPress && state.triggerLongPress);
            state.triggerShortPress = false;
            state.triggerLongPress = false;
            if (trigger) {
                switch (config.action) {
                    case Action::UiSelectNext:
                        data->enc_diff = 1;
                        break;
                    case Action::UiSelectPrevious:
                        data->enc_diff = -1;
                        break;
                    case Action::UiPressSelected:
                        data->state = LV_INDEV_STATE_PRESSED;
                        break;
                    case Action::AppClose:
                        // TODO: implement
                        break;
                }
            }
        }
        self->mutex.unlock();
    }
}

void ButtonControl::updatePin(std::vector<PinConfiguration>::const_reference configuration, std::vector<PinState>::reference state) {
    if (tt::hal::gpio::getLevel(configuration.pin)) { // if pressed
        if (state.pressState) {
            // check time for long press trigger
            auto time_passed = tt::kernel::getMillis() - state.pressStartTime;
            if (time_passed > 500) {
                // state.triggerLongPress = true;
            }
        } else {
            state.pressStartTime = tt::kernel::getMillis();
            state.pressState = true;
        }
    } else { // released
        if (state.pressState) {
            auto time_passed = tt::kernel::getMillis() - state.pressStartTime;
            if (time_passed < 500) {
                TT_LOG_D(TAG, "Trigger short press");
                state.triggerShortPress = true;
            }
            state.pressState = false;
        }
    }
}

void ButtonControl::driverThreadMain() {
    while (!shouldInterruptDriverThread()) {
        if (mutex.lock(100)) {
            for (int i = 0; i < pinConfigurations.size(); i++) {
                updatePin(pinConfigurations[i], pinStates[i]);
            }
            mutex.unlock();
        }
        tt::kernel::delayMillis(5);
    }
}

bool ButtonControl::shouldInterruptDriverThread() const {
    bool interrupt = false;
    if (mutex.lock(50 / portTICK_PERIOD_MS)) {
        interrupt = interruptDriverThread;
        mutex.unlock();
    }
    return interrupt;
}

void ButtonControl::startThread() {
    TT_LOG_I(TAG, "Start");

    mutex.lock();

    interruptDriverThread = false;

    driverThread = std::make_shared<tt::Thread>("ButtonControl", 4096, [this] {
        driverThreadMain();
        return 0;
    });

    driverThread->start();

    mutex.unlock();
}

void ButtonControl::stopThread() {
    TT_LOG_I(TAG, "Stop");

    mutex.lock();
    interruptDriverThread = true;
    mutex.unlock();

    driverThread->join();

    mutex.lock();
    driverThread = nullptr;
    mutex.unlock();
}

bool ButtonControl::startLvgl(lv_display_t* display) {
    if (deviceHandle != nullptr) {
        return false;
    }

    startThread();

    deviceHandle = lv_indev_create();
    lv_indev_set_type(deviceHandle, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_driver_data(deviceHandle, this);
    lv_indev_set_read_cb(deviceHandle, readCallback);

    return true;
}

bool ButtonControl::stopLvgl() {
    if (deviceHandle == nullptr) {
        return false;
    }

    lv_indev_delete(deviceHandle);
    deviceHandle = nullptr;

    stopThread();

    return true;
}
