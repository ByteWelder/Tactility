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
        float xfac = 0;    // Calibration slope X
        float yfac = 0;    // Calibration slope Y
        int16_t xoff = 0;  // Calibration offset X
        int16_t yoff = 0;  // Calibration offset Y
    };

    explicit SoftXpt2046Touch(std::unique_ptr<Configuration> config);
    ~SoftXpt2046Touch() override = default;

    std::string getName() const override { return "SoftXPT2046"; }
    std::string getDescription() const override { return "Software SPI touch driver for XPT2046"; }

    bool start(lv_display_t* display) override;
    bool stop() override;
    lv_indev_t* getLvglIndev() override { return indev; }

private:
    static void readCallback(lv_indev_t* indev, lv_indev_data_t* data);

    std::unique_ptr<Configuration> config;
    XPT2046_TouchscreenSOFTSPI<CYD_TOUCH_MISO_PIN, CYD_TOUCH_MOSI_PIN, CYD_TOUCH_SCK_PIN, 0> touch;
    lv_indev_t* indev = nullptr;
};
