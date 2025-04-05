#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_st7789.h>
#include <esp_lcd_ili9341.h>
#include <esp_lcd_types.h>
#include <lvgl.h>
#include <functional>

namespace tt::hal::display {

class I80Display final : public tt::hal::display::DisplayDevice {
public:
    enum class PanelType {
        ST7789,
        ILI9341
        // Add more panel types as needed
    };

    class Configuration {
    public:
        Configuration(
            gpio_num_t csPin,
            gpio_num_t dcPin,
            gpio_num_t wrPin,
            const int* dataPins, // Array of data pins (size determined by busWidth)
            unsigned int horizontalResolution,
            unsigned int verticalResolution,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch,
            PanelType panelType = PanelType::ST7789,
            unsigned int busWidth = 8,  // Changed to unsigned int
            bool swapXY = false,
            bool mirrorX = false,
            bool mirrorY = false,
            bool invertColor = false,
            uint32_t bufferSize = 0 // 0 means default (1/10 screen size)
        ) : csPin(csPin),
            dcPin(dcPin),
            wrPin(wrPin),
            horizontalResolution(horizontalResolution),
            verticalResolution(verticalResolution),
            panelType(panelType),
            busWidth(busWidth),
            swapXY(swapXY),
            mirrorX(mirrorX),
            mirrorY(mirrorY),
            invertColor(invertColor),
            bufferSize(bufferSize),
            touch(std::move(touch)) {
            // Copy data pins based on busWidth
            for (int i = 0; i < busWidth && i < 16; i++) {
                this->dataPins[i] = dataPins[i];
            }
            for (int i = busWidth; i < 16; i++) {
                this->dataPins[i] = GPIO_NUM_NC; // Fill unused pins
            }
        }

        gpio_num_t csPin;
        gpio_num_t dcPin;
        gpio_num_t wrPin;
        gpio_num_t resetPin = GPIO_NUM_NC;
        int dataPins[16]; // Max 16-bit bus, filled based on busWidth
        unsigned int pixelClockFrequency = 20'000'000; // Default 20 MHz
        size_t transactionQueueDepth = 10;
        unsigned int horizontalResolution;
        unsigned int verticalResolution;
        PanelType panelType;
        unsigned int busWidth; // Changed to unsigned int
        bool swapXY;
        bool mirrorX;
        bool mirrorY;
        bool invertColor;
        uint32_t bufferSize; // In pixels, 0 for default
        lcd_rgb_element_order_t rgbElementOrder = LCD_RGB_ELEMENT_ORDER_RGB;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        std::function<void(uint8_t)> _Nullable backlightDutyFunction = nullptr;
    };

private:
    std::unique_ptr<Configuration> configuration;
    esp_lcd_i80_bus_handle_t i80Bus = nullptr;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;

public:
    explicit I80Display(std::unique_ptr<Configuration> inConfiguration)
        : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    std::string getName() const final {
        switch (configuration->panelType) {
            case PanelType::ST7789: return "ST7789-I80";
            case PanelType::ILI9341: return "ILI9341-I80";
            default: return "Unknown-I80";
        }
    }

    std::string getDescription() const final { return "I80-based display"; }

    bool start() final;
    bool stop() final;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable createTouch() final {
        return configuration->touch;
    }

    void setBacklightDuty(uint8_t backlightDuty) final {
        if (configuration->backlightDutyFunction) {
            configuration->backlightDutyFunction(backlightDuty);
        }
    }

    bool supportsBacklightDuty() const final {
        return configuration->backlightDutyFunction != nullptr;
    }

    void setGammaCurve(uint8_t index) final;
    uint8_t getGammaCurveCount() const final { return 4; }

    lv_display_t* _Nullable getLvglDisplay() const final { return displayHandle; }
};

} // namespace tt::hal::display
