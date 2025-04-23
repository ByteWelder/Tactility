#pragma once
#
#include "Tactility/hal/display/DisplayDevice.h"
#include "YellowTouch.h"
#include <i80Display.h>
#include <memory>
#include "driver/gpio.h"
#include "lvgl.h"

namespace tt::hal::display {

class YellowDisplay : public DisplayDevice {
public:
    struct Configuration {
        int pclkHz;                     // Pixel clock frequency
        gpio_num_t csPin;               // Chip Select
        gpio_num_t dcPin;               // Data/Command
        gpio_num_t wrPin;               // Write strobe for i80
        gpio_num_t rstPin;              // Reset pin (optional)
        gpio_num_t backlightPin;        // Backlight control pin (PWM)
        gpio_num_t dataPins[8];         // 8-bit parallel data pins
        int horizontalResolution;       // Display width
        int verticalResolution;         // Display height
        uint32_t bufferSize;            // Buffer size in pixels (0 for default)
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;

        // Initial orientation settings (set at start, no runtime changes)
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

    std::string getName() const override { return "YellowDisplay"; }
    std::string getDescription() const override { return "ST7789 display for CYD-2432S022C"; }

private:
    std::unique_ptr<Configuration> config;
    std::unique_ptr<I80Display> i80Display;
    bool isStarted;
    static bool lvglInitialized;
};

std::shared_ptr<DisplayDevice> createDisplay();

} // namespace tt::hal::display
