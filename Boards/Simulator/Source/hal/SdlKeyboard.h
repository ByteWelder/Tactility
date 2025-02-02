#pragma once

#include <Tactility/hal/Keyboard.h>
#include <Tactility/TactilityCore.h>

class SdlKeyboard final : public tt::hal::Keyboard {
private:
    lv_indev_t* _Nullable handle = nullptr;

public:

    std::string getName() const final { return "SDL Keyboard"; }
    std::string getDescription() const final { return "SDL keyboard device"; }

    bool start(lv_display_t* display) override {
        handle = lv_sdl_keyboard_create();
        return handle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    bool isAttached() const override { return true; }

    lv_indev_t* _Nullable getLvglIndev() override { return handle; }
};

std::shared_ptr<tt::hal::Keyboard> createKeyboard() {
    return std::make_shared<SdlKeyboard>();
}
