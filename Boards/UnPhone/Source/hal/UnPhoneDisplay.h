#pragma once

#include <esp_lcd_types.h>
#include <lvgl.h>
#include <Tactility/hal/display/Display.h>

class UnPhoneDisplay : public tt::hal::display::Display {

private:

    lv_display_t* displayHandle = nullptr;
    uint8_t* buffer = nullptr;

public:

    std::string getName() const final { return "HX8357"; }
    std::string getDescription() const final { return "SPI display"; }

    bool start() override;

    bool stop() override;

    std::shared_ptr<tt::hal::touch::Touch> _Nullable createTouch() override;

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::display::Display> createDisplay();
