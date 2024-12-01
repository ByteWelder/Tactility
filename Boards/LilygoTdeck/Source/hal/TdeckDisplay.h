#pragma once

#include <esp_lcd_types.h>
#include "lvgl.h"
#include "hal/Display.h"

extern lv_disp_t* displayHandle;

class TdeckDisplay : public tt::hal::Display {

private:

    esp_lcd_panel_io_handle_t iohandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;
    uint8_t lastBacklightDuty = 255;
    bool poweredOn = false;

public:

    bool start() override;

    bool stop() override;

    void setPowerOn(bool turnOn) override;
    bool isPoweredOn() const override { return poweredOn; };
    bool supportsPowerControl() const override { return true; }

    tt::hal::Touch* _Nullable getTouch() override;

    void setBacklightDuty(uint8_t backlightDuty) override;
    uint8_t getBacklightDuty() const override { return lastBacklightDuty; }
    bool supportsBacklightDuty() const override { return true; }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }

private:

    static bool startBacklight();
};

tt::hal::Display* createDisplay();
