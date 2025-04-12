#pragma once

#include "Tactility/hal/touch/TouchDevice.h"
#include "YellowDisplayConstants.h"
#include "XPT2046_TouchscreenSOFTSPI.h"
#include <lvgl.h>

class SoftXpt2046Touch : public tt::hal::touch::TouchDevice {
public:
    struct Configuration {
        uint16_t xMax;
        uint16_t yMax;
        bool swapXy;
        bool mirrorX;
        bool mirrorY;
        uint16_t xMinRaw;  // Raw min X from calibration
        uint16_t xMaxRaw;  // Raw max X from calibration
        uint16_t yMinRaw;  // Raw min Y from calibration
        uint16_t yMaxRaw;  // Raw max Y from calibration

        Configuration(
            uint16_t xMax, uint16_t yMax, bool swapXy, bool mirrorX, bool mirrorY,
            uint16_t xMinRaw, uint16_t xMaxRaw, uint16_t yMinRaw, uint16_t yMaxRaw
        ) : xMax(xMax), yMax(yMax), swapXy(swapXy), mirrorX(mirrorX), mirrorY(mirrorY),
            xMinRaw(xMinRaw), xMaxRaw(xMaxRaw), yMinRaw(yMinRaw), yMaxRaw(yMaxRaw) {}
    };

    explicit SoftXpt2046Touch(std::unique_ptr<Configuration> configuration);
    ~SoftXpt2046Touch() override = default;

    std::string getName() const override { return "SoftXPT2046"; }
    std::string getDescription() const override { return "Software SPI touch driver for XPT2046"; }

    bool init();  // For YellowDisplay.cpp
    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* getLvglIndev() override { return indev; }

private:
    bool read(lv_indev_data_t* data);  // Internal read function
    std::unique_ptr<Configuration> configuration;
    XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0> touch;
    lv_indev_t* indev = nullptr;
};
