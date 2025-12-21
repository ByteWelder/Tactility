#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/display/DisplayDriver.h>
#include <Tactility/Mutex.h>
#include <Tactility/lvgl/LvglSync.h>

#include "DeviceLovyan.h"

namespace tt::hal::display {

class LovyanDisplay : public DisplayDevice {
    std::shared_ptr<tt::hal::touch::TouchDevice> touchDevice;
public:
    std::string getName() const final { return "Lovyan ILI9341"; }
    std::string getDescription() const final { return "LovyanGFX ILI9341 + XPT2046 (SPI)"; }

    bool start() override;

    bool stop() override;

    bool supportsLvgl() const override { return true; }

    bool startLvgl() override;

    bool stopLvgl() override;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override;

    lv_display_t* _Nullable getLvglDisplay() const override { return lovyan_get_display(); }

    std::shared_ptr<tt::hal::display::DisplayDriver> _Nullable nativeDisplay;

    bool supportsDisplayDriver() const override { return true; }
    std::shared_ptr<tt::hal::display::DisplayDriver> _Nullable getDisplayDriver() override {
        if (!nativeDisplay) {
            class LovyanDisplayDriver : public tt::hal::display::DisplayDriver {
            public:
                tt::hal::display::ColorFormat getColorFormat() const override { return tt::hal::display::ColorFormat::RGB565; }
                uint16_t getPixelWidth() const override { return getLovyan().width(); }
                uint16_t getPixelHeight() const override { return getLovyan().height(); }
                bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) override {
                    auto &lcd = getLovyan();
                    int w = xEnd - xStart + 1;
                    int h = yEnd - yStart + 1;
                    lcd.startWrite();
                    lcd.setAddrWindow(xStart, yStart, w, h);
                    lcd.writePixelsDMA((uint16_t*)pixelData, w * h, true);
                    lcd.endWrite();
                    return true;
                }
                std::shared_ptr<tt::Lock> getLock() const override { return tt::lvgl::getSyncLock(); }
            };
            nativeDisplay = std::make_shared<LovyanDisplayDriver>();
        }
        return nativeDisplay;
    }

    void setBacklightDuty(uint8_t backlightDuty) override { lovyan_set_backlight(backlightDuty); }
    bool supportsBacklightDuty() const override { return true; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

} // namespace tt::hal::display
