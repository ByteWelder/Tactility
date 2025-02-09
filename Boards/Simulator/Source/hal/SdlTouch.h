#pragma once

#include "Tactility/hal/touch/TouchDevice.h"
#include <Tactility/TactilityCore.h>

class SdlTouch final : public tt::hal::touch::TouchDevice {
private:
    lv_indev_t* _Nullable handle = nullptr;

public:

    std::string getName() const final { return "SDL Pointer"; }
    std::string getDescription() const final { return "SDL mouse/touch pointer device"; }

    bool start(lv_display_t* display) override {
        handle = lv_sdl_mouse_create();
        return handle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    lv_indev_t* _Nullable getLvglIndev() override { return handle; }
};

