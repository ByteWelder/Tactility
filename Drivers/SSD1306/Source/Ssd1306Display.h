#pragma once

#include <EspLcdDisplay.h>
#include <Tactility/hal/display/DisplayDevice.h>

#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_types.h>
#include <functional>
#include <lvgl.h>

class Ssd1306Display final : public EspLcdDisplay {

public:

    class Configuration {

    public:

        Configuration(
            i2c_port_t port,
            uint8_t deviceAddress,
            gpio_num_t resetPin,
            unsigned int horizontalResolution,  // Typically 128
            unsigned int verticalResolution,    // 32 or 64
            std::shared_ptr<tt::hal::touch::TouchDevice> touch = nullptr,
            bool invertColor = false
        ) : port(port),
            deviceAddress(deviceAddress),
            resetPin(resetPin),
            horizontalResolution(horizontalResolution),
            verticalResolution(verticalResolution),
            invertColor(invertColor),
            touch(std::move(touch))
        {}

        i2c_port_t port;
        uint8_t deviceAddress;
        gpio_num_t resetPin = GPIO_NUM_NC;
        unsigned int horizontalResolution;
        unsigned int verticalResolution;
        bool invertColor = false;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        uint32_t bufferSize = 0; // Size in pixel count. 0 means default, which is full screen for monochrome
        int gapX = 0;
        int gapY = 0;

        // Debug helpers (runtime toggles)
        bool debugDumpPxMap = true;
        bool debugForceFullPageWrites = false;
    };

private:

    std::unique_ptr<Configuration> configuration;

    bool createIoHandle(esp_lcd_panel_io_handle_t& ioHandle) override;

    bool createPanelHandle(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t& panelHandle) override;

    lvgl_port_display_cfg_t getLvglPortDisplayConfig(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t panelHandle) override;

public:

    explicit Ssd1306Display(std::unique_ptr<Configuration> inConfiguration) :
        EspLcdDisplay(nullptr),
        configuration(std::move(inConfiguration))
    {
        assert(configuration != nullptr);
        if (configuration->bufferSize == 0) {
            configuration->bufferSize = configuration->horizontalResolution * configuration->verticalResolution;
        }

        // Apply gap offsets to the driver
        setDriverColumnOffset(configuration->gapX);
    }

    std::string getName() const override { return "SSD1306"; }

    std::string getDescription() const override { return "SSD1306 monochrome OLED display"; }

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override { return configuration->touch; }

    void setBacklightDuty(uint8_t backlightDuty) override {
        // SSD1306 does not have backlight control
    }

    bool supportsBacklightDuty() const override { return false; }

    void setGammaCurve(uint8_t index) override {
        // SSD1306 does not support gamma curves
    }

    uint8_t getGammaCurveCount() const override { return 0; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
