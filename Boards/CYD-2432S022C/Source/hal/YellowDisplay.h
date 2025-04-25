#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <i80Display.h>
#include <memory>

class YellowDisplay : public tt::hal::display::I80Display {
public:
    using I80Display::I80Display;
    void setBacklightDuty(uint8_t duty) override;
    bool supportsBacklightDuty() const override { return true; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
