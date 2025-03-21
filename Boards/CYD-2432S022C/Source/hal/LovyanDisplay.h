#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include "lvgl.h"
#include <memory>

class LovyanGFXDisplay;  // Forward declaration

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

class LovyanGFXDisplay : public tt::hal::display::DisplayDevice {
public:
    // Existing methods...
    bool start() override;
    bool stop() override;
    void setBacklightDuty(uint8_t backlightDuty) override;
    bool supportsBacklightDuty() const override;
    lv_display_t* getLvglDisplay() const override;
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override;
    std::string getName() const override;
    std::string getDescription() const override;

    // New method to set rotation
    void setRotation(lv_display_rotation_t rotation);
};
