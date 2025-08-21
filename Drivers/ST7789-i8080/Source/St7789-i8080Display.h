#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <driver/gpio.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_types.h>
#include <functional>
#include <lvgl.h>

class St7789I8080Display : public tt::hal::display::DisplayDevice {
public:
    class Configuration {
    public:
        Configuration(
            gpio_num_t wr,
            gpio_num_t dc,
            gpio_num_t cs,
            gpio_num_t rst,
            gpio_num_t backlight,
            uint16_t horizontalResolution,
            uint16_t verticalResolution,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch,
            uint32_t pixelClockHz = 20'000'000,
            bool swapXY = false,
            bool mirrorX = false,
            bool mirrorY = false,
            bool invertColor = true,
            uint32_t bufferSize = 0,
            bool backlightOnLevel = true
        ) : pin_wr(wr),
            pin_dc(dc),
            pin_cs(cs),
            pin_rst(rst),
            pin_backlight(backlight),
            dataPins{},
            busWidth(8),
            horizontalResolution(horizontalResolution),
            verticalResolution(verticalResolution),
            pixelClockHz(pixelClockHz),
            swapXY(swapXY),
            mirrorX(mirrorX),
            mirrorY(mirrorY),
            invertColor(invertColor),
            bufferSize(bufferSize),
            backlightOnLevel(backlightOnLevel),
            touch(std::move(touch)),
            backlightDutyFunction(nullptr)
        {
            for (int i = 0; i < 16; i++) {
                dataPins[i] = GPIO_NUM_NC;
            }
        }

        // I8080 Bus configuration
        gpio_num_t pin_wr;
        gpio_num_t pin_dc;
        gpio_num_t pin_cs;
        gpio_num_t pin_rst;
        gpio_num_t pin_backlight;
        
        // Data pins (8-bit or 16-bit)
        gpio_num_t dataPins[16];
        uint8_t busWidth;  // 8 or 16
        
        // Display properties
        uint16_t horizontalResolution;
        uint16_t verticalResolution;
        uint32_t pixelClockHz;
        
        // Display orientation/mirroring
        bool swapXY;
        bool mirrorX;
        bool mirrorY;
        bool invertColor;
        
        // Buffer configuration
        uint32_t bufferSize; // Size in pixel count. 0 means default, which is 1/10 of the screen size
        
        // Backlight configuration
        bool backlightOnLevel;  // true = active high, false = active low
        
        // Touch device
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        
        // Backlight duty function (optional)
        std::function<void(uint8_t)> _Nullable backlightDutyFunction;

        void setDataPins8Bit(gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
                             gpio_num_t d4, gpio_num_t d5, gpio_num_t d6, gpio_num_t d7) {
            busWidth = 8;
            dataPins[0] = d0; dataPins[1] = d1; dataPins[2] = d2; dataPins[3] = d3;
            dataPins[4] = d4; dataPins[5] = d5; dataPins[6] = d6; dataPins[7] = d7;
        }

        void setDataPins16Bit(gpio_num_t d0, gpio_num_t d1, gpio_num_t d2, gpio_num_t d3,
                              gpio_num_t d4, gpio_num_t d5, gpio_num_t d6, gpio_num_t d7,
                              gpio_num_t d8, gpio_num_t d9, gpio_num_t d10, gpio_num_t d11,
                              gpio_num_t d12, gpio_num_t d13, gpio_num_t d14, gpio_num_t d15) {
            busWidth = 16;
            dataPins[0] = d0; dataPins[1] = d1; dataPins[2] = d2; dataPins[3] = d3;
            dataPins[4] = d4; dataPins[5] = d5; dataPins[6] = d6; dataPins[7] = d7;
            dataPins[8] = d8; dataPins[9] = d9; dataPins[10] = d10; dataPins[11] = d11;
            dataPins[12] = d12; dataPins[13] = d13; dataPins[14] = d14; dataPins[15] = d15;
        }
    };

private:
    std::unique_ptr<Configuration> configuration;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_i80_bus_handle_t i80Bus = nullptr;
    lv_display_t* displayHandle = nullptr;

public:
    explicit St7789I8080Display(std::unique_ptr<Configuration> inConfiguration) 
        : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    // Tactility DisplayDevice interface
    std::string getName() const final { return "ST7789-I8080"; }
    std::string getDescription() const final { return "ST7789 display via Intel 8080 interface"; }
    bool start() final;
    bool stop() final;
    
    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() override { 
        return configuration->touch; 
    }
    
    void setBacklightDuty(uint8_t backlightDuty) final {
        if (configuration->backlightDutyFunction != nullptr) {
            configuration->backlightDutyFunction(backlightDuty);
        } else {
            // Simple on/off backlight control
            setBacklight(backlightDuty > 128);
        }
    }
    
    bool supportsBacklightDuty() const final { 
        return configuration->backlightDutyFunction != nullptr; 
    }
    
    void setGammaCurve(uint8_t index) final;
    uint8_t getGammaCurveCount() const final { return 4; }
    
    lv_display_t* _Nullable getLvglDisplay() const final { return displayHandle; }
    
    // Additional utility functions
    uint16_t getWidth() const { return configuration->horizontalResolution; }
    uint16_t getHeight() const { return configuration->verticalResolution; }
    
    void setBacklight(bool on);
};

std::shared_ptr<tt::hal::display::DisplayDevice> createI8080Display();
