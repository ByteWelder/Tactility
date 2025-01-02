#pragma once

#include "hal/Touch.h"
#include "TactilityCore.h"
#include <esp_lcd_touch.h>

class YellowTouch : public tt::hal::Touch {
private:
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_touch_handle_t touchHandle = nullptr;
    lv_indev_t* _Nullable deviceHandle = nullptr;
    void cleanup();
public:
    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};
