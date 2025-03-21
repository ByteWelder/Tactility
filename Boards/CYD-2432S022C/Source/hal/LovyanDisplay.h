#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include "lvgl.h"
#include <memory>
#include <LovyanGFX.h>

class LovyanGFXDisplay;

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

class LovyanGFXDisplay : public tt::hal::display::DisplayDevice {
public:
    struct Configuration {
        uint16_t width;
        uint16_t height;
        std::shared_ptr<tt::hal::touch::TouchDevice> touch;
        Configuration(uint16_t width, uint16_t height, std::shared_ptr<tt::hal::touch::TouchDevice> touch)
            : width(width), height(height), touch(std::move(touch)) {}
    };

    explicit LovyanGFXDisplay(std::unique_ptr<Configuration> config);
    ~LovyanGFXDisplay() override;

    bool start() override;
    bool stop() override;
    void setBacklightDuty(uint8_t backlightDuty) override;
    bool supportsBacklightDuty() const override;
    lv_display_t* getLvglDisplay() const override;
    std::shared_ptr<tt::hal::touch::TouchDevice> createTouch() override;
    std::string getName() const override;
    std::string getDescription() const override;
    void setRotation(lv_display_rotation_t rotation);

private:
    std::unique_ptr<Configuration> configuration;
    class LGFX_CYD_2432S022C : public lgfx::LGFX_Device {
    public:
        lgfx::Panel_ST7789 _panel_instance;
        lgfx::Bus_Parallel8 _bus_instance;
        lgfx::Light_PWM _light_instance;

        LGFX_CYD_2432S022C();
    };
    LGFX_CYD_2432S022C lcd;
    lv_display_t* lvglDisplay = nullptr;
    bool isStarted = false;
};
