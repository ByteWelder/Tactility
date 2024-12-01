#pragma once

#include "lvgl.h"
#include "hal/Display.h"
#include "esp_lcd_panel_io.h"

extern lv_disp_t* displayHandle;

class YellowDisplay : public tt::hal::Display {

private:

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;
    uint8_t lastBacklightDuty = 255;

public:

    bool start() override;

    bool stop() override;

    void setPowerOn(bool turnOn) override {}
    bool isPoweredOn() const override { return true; };
    bool supportsPowerControl() const override { return false; }

    tt::hal::Touch* _Nullable createTouch() override;

    void setBacklightDuty(uint8_t backlightDuty) override;
    uint8_t getBacklightDuty() const override { return lastBacklightDuty; }
    bool supportsBacklightDuty() const override { return true; }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

tt::hal::Display* createDisplay();
