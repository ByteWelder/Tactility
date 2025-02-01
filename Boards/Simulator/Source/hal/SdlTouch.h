#pragma once

#include <Tactility/hal/Touch.h>
#include <Tactility/TactilityCore.h>

class SdlTouch : public tt::hal::Touch {
private:
    lv_indev_t* _Nullable handle = nullptr;

public:
    bool start(lv_display_t* display) override {
        handle = lv_sdl_mouse_create();
        return handle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    lv_indev_t* _Nullable getLvglIndev() override { return handle; }
};

