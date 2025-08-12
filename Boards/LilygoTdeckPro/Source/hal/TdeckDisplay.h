#pragma once

#include "Tactility/hal/display/DisplayDevice.h"

#include <esp_lcd_panel_io.h>
#include <esp_lcd_types.h>
#include <lvgl.h>

class TdeckDisplay final : public tt::hal::display::DisplayDevice {

    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;

public:

    explicit TdeckDisplay() {}

    std::string getName() const { return "Epaper"; }
    std::string getDescription() const { return "Epaper display"; }

    bool start();

    bool stop();

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable createTouch() { return nullptr; }

    lv_display_t* _Nullable getLvglDisplay() const { return displayHandle; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
