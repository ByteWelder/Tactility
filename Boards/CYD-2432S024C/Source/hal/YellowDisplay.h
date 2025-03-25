#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <memory>
#include "esp_err.h"
#include "lvgl.h"

namespace tt::hal::display {

class YellowDisplay : public DisplayDevice {
public:
    struct Configuration {
        int pclkHz;
        gpio_num_t csPin;
        gpio_num_t dcPin;
        gpio_num_t wrPin;
        gpio_num_t rstPin;
        gpio_num_t backlightPin;
        gpio_num_t dataPins[8];
        int horizontalResolution;
        int verticalResolution;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;

        // Optional settings
        bool mirrorX = false;
        bool mirrorY = false;
        bool swapXY = false;
    };

    explicit YellowDisplay(std::unique_ptr<Configuration> config);
    ~YellowDisplay() override;

    lv_display_t* getLvglDisplay() const override;

    void setRotation(lv_display_rotation_t rotation);

private:
    std::unique_ptr<Configuration> config;
    esp_lcd_panel_handle_t panelHandle;
    lv_display_t* lvglDisplay;

    void initialize();
};

std::shared_ptr<DisplayDevice> createDisplay();

} // namespace tt::hal::display
