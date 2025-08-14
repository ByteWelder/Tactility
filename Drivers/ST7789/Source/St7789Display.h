#pragma once

#include <Tactility/hal/display/DisplayDevice.h>

#include <EspLcdNativeDisplay.h>

#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_types.h>
#include <functional>
#include <lvgl.h>

class St7789Display final : public tt::hal::display::DisplayDevice {

public:

    class Configuration {

    public:

        Configuration(
            esp_lcd_spi_bus_handle_t spi_bus_handle,
            gpio_num_t csPin,
            gpio_num_t dcPin,
            unsigned int horizontalResolution,
            unsigned int verticalResolution,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch,
            bool swapXY = false,
            bool mirrorX = false,
            bool mirrorY = false,
            bool invertColor = false,
            uint32_t bufferSize = 0 // Size in pixel count. 0 means default, which is 1/10 of the screen size
        ) : spiBusHandle(spi_bus_handle),
            csPin(csPin),
            dcPin(dcPin),
            horizontalResolution(horizontalResolution),
            verticalResolution(verticalResolution),
            swapXY(swapXY),
            mirrorX(mirrorX),
            mirrorY(mirrorY),
            invertColor(invertColor),
            bufferSize(bufferSize),
            touch(std::move(touch))
        {}

        esp_lcd_spi_bus_handle_t spiBusHandle;
        gpio_num_t csPin;
        gpio_num_t dcPin;
        gpio_num_t resetPin = GPIO_NUM_NC;
        unsigned int pixelClockFrequency = 80'000'000; // Hertz
        size_t transactionQueueDepth = 10;
        unsigned int horizontalResolution;
        unsigned int verticalResolution;
        bool swapXY = false;
        bool mirrorX = false;
        bool mirrorY = false;
        bool invertColor = false;
        uint32_t bufferSize = 0; // Size in pixel count. 0 means default, which is 1/10 of the screen size
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        std::function<void(uint8_t)> _Nullable backlightDutyFunction = nullptr;
    };

private:

    std::unique_ptr<Configuration> configuration;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;

public:

    explicit St7789Display(std::unique_ptr<Configuration> inConfiguration) : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    std::string getName() const override { return "ST7789"; }
    std::string getDescription() const override { return "ST7789 display"; }

    bool start() override;

    bool stop() override;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable createTouch() override { return configuration->touch; }

    void setBacklightDuty(uint8_t backlightDuty) override {
        if (configuration->backlightDutyFunction != nullptr) {
            configuration->backlightDutyFunction(backlightDuty);
        }
    }

    bool supportsBacklightDuty() const override { return configuration->backlightDutyFunction != nullptr; }

    void setGammaCurve(uint8_t index) override;
    uint8_t getGammaCurveCount() const override { return 4; };

    // region LVGL

    bool supportsLvgl() const override { return true; }

    bool startLvgl() override;

    bool stopLvgl() override;

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }

    // endregion

    // region NativedDisplay

    bool supportsNativeDisplay() const override { return true; }

    std::shared_ptr<tt::hal::display::NativeDisplay> getNativeDisplay() override {
        assert(displayHandle == nullptr); // Still attached to LVGL context. Call stopLvgl() first.

        uint16_t width = configuration->swapXY ? configuration->verticalResolution : configuration->horizontalResolution;
        uint16_t height = configuration->swapXY ? configuration->horizontalResolution : configuration->verticalResolution;

        return std::make_shared<tt::hal::display::EspLcdNativeDisplay>(
            panelHandle,
            tt::hal::display::ColorFormat::RGB565,
            width,
            height
        );
    }

    // endregion
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
