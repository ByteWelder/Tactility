#pragma once

#include "Tactility/hal/touch/TouchDevice.h"
#include <Tactility/TactilityCore.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_touch.h>

class TdeckTouch : public tt::hal::touch::TouchDevice {

private:

    std::string getName() const final { return "GT911"; }
    std::string getDescription() const final { return "I2C Touch Driver"; }

    esp_lcd_panel_io_handle_t _Nullable ioHandle = nullptr;
    esp_lcd_touch_handle_t _Nullable touchHandle = nullptr;
    lv_indev_t* _Nullable deviceHandle = nullptr;
    void cleanup();

public:

    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};
