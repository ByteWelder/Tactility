#pragma once

#include <Tactility/hal/Touch.h>
#include <Tactility/TactilityCore.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_touch.h>

class TdeckTouch : public tt::hal::Touch {
private:
    esp_lcd_panel_io_handle_t _Nullable ioHandle = nullptr;
    esp_lcd_touch_handle_t _Nullable touchHandle = nullptr;
    lv_indev_t* _Nullable deviceHandle = nullptr;
    void cleanup();
public:
    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};
