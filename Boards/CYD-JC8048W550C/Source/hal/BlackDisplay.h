#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <esp_lcd_types.h>
#include <lvgl.h>

class BlackDisplay : public tt::hal::display::DisplayDevice {

private:

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;

public:

    std::string getName() const final { return "ST7262"; }
    std::string getDescription() const final { return "RGB Display"; }

    bool start() override;

    bool stop() override;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable createTouch() override;

    void setBacklightDuty(uint8_t backlightDuty) override;
    bool supportsBacklightDuty() const override { return true; }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
