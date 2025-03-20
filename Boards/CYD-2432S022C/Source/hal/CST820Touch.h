#pragma once

#include <Tactility/hal/touch/TouchDevice.h>
#include "CYD2432S022CConstants.h"
#include <lvgl.h> // Add LVGL header for lv_indev_t and lv_display_t
#include <memory>
#include <string>

std::shared_ptr<tt::hal::touch::TouchDevice> createCST820Touch();

class CST820Touch : public tt::hal::touch::TouchDevice {
public:
    struct Configuration {
        i2c_port_t i2c_port;
        int width;
        int height;
    };

    CST820Touch(std::unique_ptr<Configuration> config);
    ~CST820Touch() override = default;

    // Methods from Device
    std::string getName() const override { return "CST820 Touch"; }
    std::string getDescription() const override { return "CST820 Capacitive Touch Controller"; }

    // Methods from TouchDevice
    bool start(lv_display_t* display) override;
    bool stop() override;
    void read(lv_indev_t* indev, lv_indev_data_t* data) override;
    lv_indev_t* getLvglIndev() override { return indev_; }
    int get_width() const override { return config_->width; }
    int get_height() const override { return config_->height; }

private:
    std::unique_ptr<Configuration> config_;
    lv_indev_t* indev_ = nullptr; // Store the LVGL input device
};
