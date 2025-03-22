#pragma once

#include "Tactility/hal/display/DisplayDevice.h"

#include "/opt/esp/idf/components/esp_lcd/rgb/include/esp_lcd_panel_rgb.h"
#include <esp_lcd_types.h>
#include <lvgl.h>

#include <utility>

class RgbDisplay : public tt::hal::display::DisplayDevice {

public:

    struct BufferConfiguration final {
        uint32_t size; // Size in pixel count. 0 means default, which is 1/15 of the screen size
        bool useSpi;
        bool doubleBuffer;
        bool bounceBufferMode;
        bool avoidTearing;
    };

    class Configuration final {
    public:

        esp_lcd_rgb_panel_config_t panelConfig;
        BufferConfiguration bufferConfiguration;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        lv_color_format_t colorFormat;
        bool swapXY;
        bool mirrorX;
        bool mirrorY;
        bool invertColor;
        std::function<void(uint8_t)> _Nullable backlightDutyFunction;

        Configuration(
            esp_lcd_rgb_panel_config_t panelConfig,
            BufferConfiguration bufferConfiguration,
            std::shared_ptr<tt::hal::touch::TouchDevice> touch,
            lv_color_format_t colorFormat,
            bool swapXY = false,
            bool mirrorX = false,
            bool mirrorY = false,
            bool invertColor = false,
            std::function<void(uint8_t)> _Nullable backlightDutyFunction = nullptr
        ) : panelConfig(panelConfig),
            bufferConfiguration(bufferConfiguration),
            touch(std::move(touch)),
            colorFormat(colorFormat),
            swapXY(swapXY),
            mirrorX(mirrorX),
            mirrorY(mirrorY),
            invertColor(invertColor),
            backlightDutyFunction(std::move(backlightDutyFunction))
        {}
    };

private:

    std::unique_ptr<Configuration> configuration = nullptr;
    esp_lcd_panel_io_handle_t ioHandle = nullptr;
    esp_lcd_panel_handle_t panelHandle = nullptr;
    lv_display_t* displayHandle = nullptr;

public:

    explicit RgbDisplay(std::unique_ptr<Configuration> inConfiguration) : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    std::string getName() const final { return "RGB Display"; }
    std::string getDescription() const final { return "RGB Display"; }

    bool start() override;

    bool stop() override;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable createTouch() final { return configuration->touch; }

    void setBacklightDuty(uint8_t backlightDuty) final {
        if (configuration->backlightDutyFunction != nullptr) {
            configuration->backlightDutyFunction(backlightDuty);
        }
    }

    bool supportsBacklightDuty() const final { return configuration->backlightDutyFunction != nullptr; }

    lv_display_t* _Nullable getLvglDisplay() const override { return displayHandle; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
