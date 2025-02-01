#pragma once

#include "SdlTouch.h"
#include <Tactility/hal/Display.h>

extern lv_disp_t* displayHandle;

class SdlDisplay : public tt::hal::Display {
public:
    bool start() override {
        return displayHandle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    tt::hal::Touch* _Nullable createTouch() override { return dynamic_cast<tt::hal::Touch*>(new SdlTouch()); }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

tt::hal::Display* createDisplay() {
    return static_cast<tt::hal::Display*>(new SdlDisplay());
}

