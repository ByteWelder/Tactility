#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <memory>
#include "esp_err.h"
#include "lvgl.h"
#include "YellowTouch.h"

namespace tt::hal::display {

class YellowDisplay : public DisplayDevice {
public:
    struct Configuration {
        int pclkHz;                     // Pixel clock frequency
        gpio_num_t csPin;               // Chip Select
        gpio_num_t dcPin;               // Data/Command
        gpio_num_t wrPin;               // Write strobe for i80
        gpio_num_t rstPin;              // Reset pin (optional)
        gpio_num_t backlightPin;        // Backlight control pin
        gpio_num_t dataPins[8];         // 8-bit parallel data pins
        int horizontalResolution;       // Display width
        int verticalResolution;         // Display height
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;  // Touch device

        // Optional initial orientation settings
        bool mirrorX = false;
        bool mirrorY = false;
        bool swapXY = false;
    };

    explicit YellowDisplay(std::unique_ptr<Configuration> config);
    ~YellowDisplay() override;

    bool start() override;
    bool stop() override;
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override;
    lv_display_t* getLvglDisplay() const override;

    void setBacklightDuty(uint8_t backlightDuty) override;
    bool supportsBacklightDuty() const override { return true; }

    void setRotation(lv_display_rotation_t rotation);  // Custom method for rotation

private:
    std::unique_ptr<Configuration> config;
    esp_lcd_panel_handle_t panelHandle;
    lv_display_t* lvglDisplay;
    bool isStarted;

    void initialize();
    void deinitialize();
};

std::shared_ptr<DisplayDevice> createDisplay();

} // namespace tt::hal::display
