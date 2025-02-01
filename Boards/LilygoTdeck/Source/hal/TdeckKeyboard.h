#pragma once

#include <Tactility/hal/Keyboard.h>
#include <Tactility/TactilityCore.h>
#include <esp_lcd_panel_io_interface.h>
#include <esp_lcd_touch.h>

class TdeckKeyboard : public tt::hal::Keyboard {
private:
    lv_indev_t* _Nullable deviceHandle = nullptr;
public:
    bool start(lv_display_t* display) override;
    bool stop() override;
    bool isAttached() const override;
    lv_indev_t* _Nullable getLvglIndev() override { return deviceHandle; }
};

tt::hal::Keyboard* createKeyboard();
