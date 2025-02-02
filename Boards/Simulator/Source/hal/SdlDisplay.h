#pragma once

#include "SdlTouch.h"
#include <Tactility/hal/Display.h>
#include <memory>

extern lv_disp_t* displayHandle;

class SdlDisplay final : public tt::hal::Display {
public:

    std::string getName() const final { return "SDL Display"; }
    std::string getDescription() const final { return ""; }

    bool start() override {
        return displayHandle != nullptr;
    }

    bool stop() override { tt_crash("Not supported"); }

    std::shared_ptr<tt::hal::Touch> _Nullable createTouch() override { return std::make_shared<SdlTouch>(); }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::Display> createDisplay() {
    return std::make_shared<SdlDisplay>();
}

