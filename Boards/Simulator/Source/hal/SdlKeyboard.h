#pragma once

#include <Tactility/hal/keyboard/KeyboardDevice.h>
#include <Tactility/TactilityCore.h>

class SdlKeyboard final : public tt::hal::keyboard::KeyboardDevice {

    lv_indev_t* _Nullable handle = nullptr;

public:

    std::string getName() const override { return "SDL Keyboard"; }
    std::string getDescription() const override { return "SDL keyboard device"; }

    bool startLvgl(lv_display_t* display) override {
        handle = lv_sdl_keyboard_create();
        return handle != nullptr;
    }

    bool stopLvgl() override { tt_crash("Not supported"); }

    bool isAttached() const override { return true; }

    lv_indev_t* _Nullable getLvglIndev() override { return handle; }
};
