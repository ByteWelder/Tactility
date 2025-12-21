#pragma once

#include <Tactility/hal/touch/TouchDevice.h>
#include <Tactility/hal/touch/TouchDriver.h>

namespace tt::hal::touch {

class LovyanTouch : public TouchDevice {
    std::shared_ptr<tt::hal::touch::TouchDriver> touchDriver;
public:
    std::string getName() const final { return "Lovyan XPT2046"; }
    std::string getDescription() const override { return "LovyanGFX XPT2046 touch (SPI)"; }

    bool start() override { return true; }
    bool stop() override { return true; }

    bool supportsLvgl() const override { return true; }
    bool startLvgl(lv_display_t* display) override;
    bool stopLvgl() override;

    lv_indev_t* _Nullable getLvglIndev() override;

    bool supportsTouchDriver() override { return true; }
    std::shared_ptr<tt::hal::touch::TouchDriver> _Nullable getTouchDriver() override;
};

} // namespace tt::hal::touch
