#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <esp_lvgl_port.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_types.h>
#include <lvgl.h>
#include <lv_st7789.h>
#include "lv_lcd_generic_mipi.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

class I8080St7789Display : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        gpio_num_t csPin;
        gpio_num_t dcPin;
        gpio_num_t wrPin;
        gpio_num_t rdPin;
        std::array<gpio_num_t, 8> dataPins;
        gpio_num_t resetPin;
        gpio_num_t backlightPin;
        unsigned int pixelClockFrequency = 16 * 1000 * 1000;   // higher = better, but max 40MHz for ST7789
        size_t transactionQueueDepth = 40;
        size_t bufferSize = 170 * 320;                         // full frame buffer

        Configuration(gpio_num_t cs, gpio_num_t dc, gpio_num_t wr, gpio_num_t rd,
                      std::array<gpio_num_t, 8> data, gpio_num_t rst, gpio_num_t bl)
            : csPin(cs), dcPin(dc), wrPin(wr), rdPin(rd),
              dataPins(data), resetPin(rst), backlightPin(bl) {}
    };

private:
    Configuration configuration;
    esp_lcd_i80_bus_handle_t i80BusHandle = nullptr;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* lvglDisplay = nullptr;

public:
    explicit I8080St7789Display(const Configuration& config) : configuration(config) {}

    // Hardware setup only
    bool initialize(lv_display_t* lvglDisplayCtx);

    // LVGL registration only (called by framework when LVGL is ready)
    bool startLvgl() override;

    lv_display_t* getLvglDisplay() const override;

    esp_lcd_panel_io_handle_t getIoHandle() const { return ioHandle; }
    
    // For LVGL flush callback
    esp_lcd_panel_handle_t getPanelHandle() const { return panelHandle; }

    // Metadata
    std::string getName() const override { return "I8080 ST7789"; }
    std::string getDescription() const override { return "I8080-based ST7789 display"; }

    // Lifecycle (no-op for now)
    bool start() override { return true; }
    bool stop() override { return true; }
    bool stopLvgl() override { return true; }

    // Capabilities
    bool supportsLvgl() const override { return true; }
    bool supportsDisplayDriver() const override { return false; }

    // No touch or external driver
    std::shared_ptr<tt::hal::touch::TouchDevice> getTouchDevice() override { return nullptr; }
    std::shared_ptr<tt::hal::display::DisplayDriver> getDisplayDriver() override { return nullptr; }
};

// Factory function for registration
std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
