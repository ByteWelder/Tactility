#pragma once

#include <Tactility/hal/Keyboard.h>
#include <Tactility/TactilityCore.h>

class SdlKeyboard : public tt::hal::Keyboard {
private:
    lv_indev_t* _Nullable handle = nullptr;

public:
    bool start(lv_display_t* display) override {
        handle = lv_sdl_keyboard_create();
        return handle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    bool isAttached() const override { return true; }

    lv_indev_t* _Nullable getLvglIndev() override { return handle; }
};

tt::hal::Keyboard* createKeyboard() {
    return static_cast<tt::hal::Keyboard*>(new SdlKeyboard());
}
