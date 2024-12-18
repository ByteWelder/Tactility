#pragma once

#include "lvgl.h"

namespace tt::hal {

class Touch;

class Display {
public:
    virtual bool start() = 0;
    virtual bool stop() = 0;

    virtual void setPowerOn(bool turnOn) = 0;
    virtual bool isPoweredOn() const = 0;
    virtual bool supportsPowerControl() const = 0;

    virtual Touch* _Nullable createTouch() = 0;

    /** Set a value in the range [0, 255] */
    virtual void setBacklightDuty(uint8_t backlightDuty) { /* NO-OP */ }
    virtual bool supportsBacklightDuty() const { return false; }

    /** Set a value in the range [0, 255] */
    virtual void setGammaCurve(uint8_t index) { /* NO-OP */ }
    virtual uint8_t getGammaCurveCount() const { return 0; };

    /** After start() returns true, this should return a valid pointer until stop() is called and returns true */
    virtual lv_display_t* _Nullable getLvglDisplay() const = 0;
};

}
