#pragma once

#include "lvgl.h"
#include <Tactility/hal/Display.h>
#include "esp_lcd_panel_io.h"

extern lv_disp_t* displayHandle;

class YellowDisplay : public tt::hal::Display {

private:

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;

public:

    std::string getName() const final { return "ILI9341"; }
    std::string getDescription() const final { return "SPI display"; }

    bool start() override;

    bool stop() override;

    std::shared_ptr<tt::hal::Touch> _Nullable createTouch() override;

    void setBacklightDuty(uint8_t backlightDuty) override;
    bool supportsBacklightDuty() const override { return true; }

    void setGammaCurve(uint8_t index) override;
    uint8_t getGammaCurveCount() const override { return 4; };

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::Display> createDisplay();
