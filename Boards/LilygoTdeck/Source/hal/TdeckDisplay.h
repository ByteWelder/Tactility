#pragma once

#include <esp_lcd_types.h>
#include <lvgl.h>
#include <Tactility/hal/Display.h>

extern lv_disp_t* displayHandle;

class TdeckDisplay : public tt::hal::Display {

private:

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;
    bool poweredOn = false;

public:

    bool start() override;

    bool stop() override;

    void setPowerOn(bool turnOn) override;
    bool isPoweredOn() const override { return poweredOn; };
    bool supportsPowerControl() const override { return true; }

    tt::hal::Touch* _Nullable createTouch() override;

    void setBacklightDuty(uint8_t backlightDuty) override;
    bool supportsBacklightDuty() const override { return true; }

    void setGammaCurve(uint8_t index) override;
    uint8_t getGammaCurveCount() const override { return 4; };

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }

private:

    static bool startBacklight();
};

tt::hal::Display* createDisplay();
