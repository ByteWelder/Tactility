#pragma once

#include "lvgl.h"
#include <Tactility/hal/Display.h>

#include <esp_lcd_types.h>

extern lv_disp_t* displayHandle;

class Core2Display : public tt::hal::Display {

private:

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;

public:

    std::string getName() const final { return "ILI9342C"; }
    std::string getDescription() const final { return "Display (ILI9342C with an ILI9341 driver)"; }

    bool start() override;

    bool stop() override;

    std::shared_ptr<tt::hal::Touch> _Nullable createTouch() override;

    bool supportsBacklightDuty() const override { return false; }

    void setGammaCurve(uint8_t index) override;
    uint8_t getGammaCurveCount() const override { return 4; };

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::Display> createDisplay();
