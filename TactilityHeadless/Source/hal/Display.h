#pragma once

#include "lvgl.h"

namespace tt::hal {

class Touch;

class Display {
public:
    [[nodiscard]] virtual bool start() = 0;
    [[nodiscard]] virtual bool stop() = 0;

    [[nodiscard]] virtual void setPowerOn(bool turnOn) = 0;
    [[nodiscard]] virtual bool isPoweredOn() const = 0;
    [[nodiscard]] virtual bool supportsPowerControl() const = 0;

    [[nodiscard]] virtual Touch* _Nullable getTouch() = 0;

    /** Set a value in the range [0, 255] */
    virtual void setBacklightDuty(uint8_t backlightDuty) = 0;
    [[nodiscard]] virtual uint8_t getBacklightDuty() const = 0;
    [[nodiscard]] virtual bool supportsBacklightDuty() const = 0;

    /** After start() returns true, this should return a valid pointer until stop() is called and returns true */
    [[nodiscard]] virtual lv_display_t* _Nullable getLvglDisplay() const = 0;
};

}
