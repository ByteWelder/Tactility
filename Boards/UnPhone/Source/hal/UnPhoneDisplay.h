#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/display/DisplayDriver.h>
#include <Tactility/Mutex.h>

#include <esp_lcd_types.h>
#include <lvgl.h>

#include "UnPhoneDisplayConstants.h"

#include <Tactility/hal/spi/Spi.h>

class UnPhoneDisplay : public tt::hal::display::DisplayDevice {

    uint8_t* _Nullable buffer = nullptr;
    lv_display_t* _Nullable lvglDisplay = nullptr;
    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable touchDevice;
    std::shared_ptr<tt::hal::display::DisplayDriver> _Nullable nativeDisplay;

    class UnPhoneDisplayDriver : public tt::hal::display::DisplayDriver {
        std::shared_ptr<tt::Lock> lock = tt::hal::spi::getLock(SPI2_HOST);
    public:
        tt::hal::display::ColorFormat getColorFormat() const override { return tt::hal::display::ColorFormat::RGB888; }
        uint16_t getPixelWidth() const override { return UNPHONE_LCD_HORIZONTAL_RESOLUTION; }
        uint16_t getPixelHeight() const override { return UNPHONE_LCD_VERTICAL_RESOLUTION; }
        bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) override;
        std::shared_ptr<tt::Lock> getLock() const override { return lock; }
    };

public:

    std::string getName() const final { return "HX8357"; }
    std::string getDescription() const final { return "SPI display"; }

    bool start() override;

    bool stop() override;

    bool supportsLvgl() const override { return true; }

    bool startLvgl() override;

    bool stopLvgl() override;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override;

    lv_display_t* _Nullable getLvglDisplay() const override { return lvglDisplay; }

    // TODO: Set to true after fixing UnPhoneDisplayDriver
    bool supportsDisplayDriver() const override { return false; }

    std::shared_ptr<tt::hal::display::DisplayDriver> _Nullable getDisplayDriver() override {
        if (nativeDisplay == nullptr) {
            nativeDisplay = std::make_shared<UnPhoneDisplayDriver>();
        }
        assert(nativeDisplay != nullptr);
        return nativeDisplay;
    }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
