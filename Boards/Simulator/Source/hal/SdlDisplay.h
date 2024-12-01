#pragma once

#include "SdlTouch.h"
#include "hal/Display.h"

extern lv_disp_t* displayHandle;

class SdlDisplay : public tt::hal::Display {
public:
    bool start() override {
        return displayHandle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    void setPowerOn(bool turnOn) override {}
    bool isPoweredOn() const override { return displayHandle != nullptr; }
    bool supportsPowerControl() const override { return false; }

    tt::hal::Touch* _Nullable createTouch() override { return dynamic_cast<tt::hal::Touch*>(new SdlTouch()); }

    void setBacklightDuty(uint8_t backlightDuty) override {}
    uint8_t getBacklightDuty() const override { return 255; }
    bool supportsBacklightDuty() const override { return false; }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

tt::hal::Display* createDisplay() {
    return dynamic_cast<tt::hal::Display*>(new SdlDisplay());
}

