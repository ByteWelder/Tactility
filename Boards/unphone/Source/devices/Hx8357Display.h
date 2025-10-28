#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/display/DisplayDriver.h>
#include <Tactility/Mutex.h>

#include <esp_lcd_types.h>
#include <lvgl.h>

#include <Tactility/hal/spi/Spi.h>

#define UNPHONE_LCD_SPI_HOST SPI2_HOST
#define UNPHONE_LCD_PIN_CS GPIO_NUM_48
#define UNPHONE_LCD_PIN_DC GPIO_NUM_47
#define UNPHONE_LCD_PIN_RESET GPIO_NUM_46
#define UNPHONE_LCD_SPI_FREQUENCY 27000000
#define UNPHONE_LCD_HORIZONTAL_RESOLUTION 320
#define UNPHONE_LCD_VERTICAL_RESOLUTION 480
#define UNPHONE_LCD_DRAW_BUFFER_HEIGHT (UNPHONE_LCD_VERTICAL_RESOLUTION / 15)
#define UNPHONE_LCD_SPI_TRANSFER_HEIGHT (UNPHONE_LCD_VERTICAL_RESOLUTION / 15)

class Hx8357Display : public tt::hal::display::DisplayDevice {

    uint8_t* _Nullable buffer = nullptr;
    lv_display_t* _Nullable lvglDisplay = nullptr;
    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable touchDevice;
    std::shared_ptr<tt::hal::display::DisplayDriver> _Nullable nativeDisplay;

    class Hx8357Driver : public tt::hal::display::DisplayDriver {
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
            nativeDisplay = std::make_shared<Hx8357Driver>();
        }
        assert(nativeDisplay != nullptr);
        return nativeDisplay;
    }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
