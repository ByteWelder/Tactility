#pragma once

#include "lvgl.h"
#include "hal/Display.h"

extern lv_disp_t* displayHandle;

class M5stackDisplay : public tt::hal::Display {

private:

    lv_display_t* displayHandle = nullptr;

public:

    bool start() override;

    bool stop() override;

    void setPowerOn(bool turnOn) override {}
    bool isPoweredOn() const override { return true; };
    bool supportsPowerControl() const override { return false; }

    tt::hal::Touch* _Nullable createTouch() override;

    void setBacklightDuty(uint8_t backlightDuty) override {}
    uint8_t getBacklightDuty() const override { return 255; }
    bool supportsBacklightDuty() const override { return false; }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

tt::hal::Display* createDisplay();
