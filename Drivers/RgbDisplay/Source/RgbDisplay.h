#pragma once

#include "Tactility/hal/display/DisplayDevice.h"

#include <esp_lcd_panel_rgb.h>
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
            backlightDutyFunction(std::move(backlightDutyFunction)) {
            if (this->bufferConfiguration.size == 0) {
                auto horizontal_resolution = panelConfig.timings.h_res;
                auto vertical_resolution = panelConfig.timings.v_res;
                this->bufferConfiguration.size = horizontal_resolution * vertical_resolution / 15;
            }
        }
    };

private:

    std::unique_ptr<Configuration> _Nullable configuration = nullptr;
    esp_lcd_panel_io_handle_t _Nullable ioHandle = nullptr;
    esp_lcd_panel_handle_t _Nullable panelHandle = nullptr;
    lv_display_t* _Nullable lvglDisplay = nullptr;
    std::shared_ptr<tt::hal::display::NativeDisplay> _Nullable nativeDisplay;

public:

    explicit RgbDisplay(std::unique_ptr<Configuration> inConfiguration) : configuration(std::move(inConfiguration)) {
        assert(configuration != nullptr);
    }

    ~RgbDisplay();

    std::string getName() const final { return "RGB Display"; }
    std::string getDescription() const final { return "RGB Display"; }

    bool start() override;

    bool stop() override;

    bool supportsLvgl() const override { return true; }

    bool startLvgl() override;

    bool stopLvgl() override;

    std::shared_ptr<tt::hal::touch::TouchDevice> _Nullable getTouchDevice() final { return configuration->touch; }

    void setBacklightDuty(uint8_t backlightDuty) final {
        if (configuration->backlightDutyFunction != nullptr) {
            configuration->backlightDutyFunction(backlightDuty);
        }
    }

    bool supportsBacklightDuty() const final { return configuration->backlightDutyFunction != nullptr; }

    lv_display_t* _Nullable getLvglDisplay() const override { return lvglDisplay; }
};

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
