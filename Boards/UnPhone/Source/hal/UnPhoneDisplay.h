#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <esp_lcd_types.h>
#include <lvgl.h>
#include <Tactility/hal/display/NativeDisplay.h>

#include "UnPhoneDisplayConstants.h"

class UnPhoneDisplay : public tt::hal::display::DisplayDevice {

    uint8_t* _Nullable buffer = nullptr;
    lv_display_t* _Nullable lvglDisplay = nullptr;
    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable touchDevice;
    std::shared_ptr<tt::hal::display::NativeDisplay> _Nullable nativeDisplay;


    class UnPhoneNativeDisplay : public tt::hal::display::NativeDisplay {
        tt::hal::display::ColorFormat getColorFormat() const override { return tt::hal::display::ColorFormat::RGB888; }
        uint16_t getPixelWidth() const override { return UNPHONE_LCD_HORIZONTAL_RESOLUTION; }
        uint16_t getPixelHeight() const override { return UNPHONE_LCD_VERTICAL_RESOLUTION; }
        bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) override;
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

    // TODO: Set to true after fixing UnPhoneNativeDisplay
    bool supportsNativeDisplay() const override { return false; }

    std::shared_ptr<tt::hal::display::NativeDisplay> _Nullable getNativeDisplay() override {
        if (nativeDisplay == nullptr) {
            nativeDisplay = std::make_shared<UnPhoneNativeDisplay>();
        }
        assert(nativeDisplay != nullptr);
        return nativeDisplay;
    }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
