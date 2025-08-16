#pragma once

#include "SdlTouch.h"
#include <Tactility/hal/display/DisplayDevice.h>

/** Hack: variable comes from LvglTask.cpp */
extern lv_disp_t* displayHandle;

class SdlDisplay final : public tt::hal::display::DisplayDevice {

public:

    std::string getName() const override { return "SDL Display"; }
    std::string getDescription() const override { return ""; }

    bool start() override {
        return displayHandle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    bool supportsLvgl() const override { return true; }
    bool startLvgl() override { return true; }
    bool stopLvgl() override { tt_crash("Not supported"); }

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override { return std::make_shared<SdlTouch>(); }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay() {
    return std::make_shared<SdlDisplay>();
}

