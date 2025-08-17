#pragma once

#include "Tactility/hal/display/DisplayDevice.h"

#include <driver/spi_common.h>
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_types.h>
#include <functional>
#include <lvgl.h>

#include <EspLcdDisplay.h>

class Ili934xDisplay final : public EspLcdDisplay {

public:

    class Configuration {

    public:

        Configuration(
            spi_host_device_t spiHostDevice,
            gpio_num_t csPin,
            gpio_num_t dcPin,
            unsigned int horizontalResolution,
            unsigned int verticalResolution,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch,
            bool swapXY = false,
            bool mirrorX = false,
            bool mirrorY = false,
            bool invertColor = false,
            uint32_t bufferSize = 0, // Size in pixel count. 0 means default, which is 1/10 of the screen size,
            lcd_rgb_element_order_t rgbElementOrder = LCD_RGB_ELEMENT_ORDER_BGR
        ) : spiHostDevice(spiHostDevice),
            csPin(csPin),
            dcPin(dcPin),
            horizontalResolution(horizontalResolution),
            verticalResolution(verticalResolution),
            swapXY(swapXY),
            mirrorX(mirrorX),
            mirrorY(mirrorY),
            invertColor(invertColor),
            bufferSize(bufferSize),
            rgbElementOrder(rgbElementOrder),
            touch(std::move(touch)
        ) {
            if (this->bufferSize == 0) {
                this->bufferSize = horizontalResolution * verticalResolution / 10;
            }
        }

        spi_host_device_t spiHostDevice;
        gpio_num_t csPin;
        gpio_num_t dcPin;
        gpio_num_t resetPin = GPIO_NUM_NC;
        unsigned int pixelClockFrequency = 40'000'000; // Hertz
        size_t transactionQueueDepth = 10;
        unsigned int horizontalResolution;
        unsigned int verticalResolution;
        bool swapXY = false;
        bool mirrorX = false;
        bool mirrorY = false;
        bool invertColor = false;
        uint32_t bufferSize = 0; // Size in pixel count. 0 means default, which is 1/10 of the screen size
        lcd_rgb_element_order_t rgbElementOrder = LCD_RGB_ELEMENT_ORDER_BGR;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        std::function<void(uint8_t)> _Nullable backlightDutyFunction = nullptr;
    };

private:

    std::unique_ptr<Configuration> configuration;

    bool createIoHandle(esp_lcd_panel_io_handle_t& outHandle) override;

    bool createPanelHandle(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t& panelHandle) override;

    lvgl_port_display_cfg_t getLvglPortDisplayConfig(esp_lcd_panel_io_handle_t ioHandle, esp_lcd_panel_handle_t panelHandle) override;

public:

    explicit Ili934xDisplay(std::unique_ptr<Configuration> inConfiguration) :
        EspLcdDisplay(tt::hal::spi::getLock(inConfiguration->spiHostDevice)),
        configuration(std::move(inConfiguration)
    ) {
        assert(configuration != nullptr);
    }

    std::string getName() const override { return "ILI934x"; }

    std::string getDescription() const override { return "ILI934x display"; }

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override { return configuration->touch; }

    void setBacklightDuty(uint8_t backlightDuty) override {
        if (configuration->backlightDutyFunction != nullptr) {
            configuration->backlightDutyFunction(backlightDuty);
        }
    }

    bool supportsBacklightDuty() const override { return configuration->backlightDutyFunction != nullptr; }

    void setGammaCurve(uint8_t index) override;

    uint8_t getGammaCurveCount() const override { return 4; };
};
