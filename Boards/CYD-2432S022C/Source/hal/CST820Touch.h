#pragma once

#include <Tactility/hal/touch/TouchDevice.h>
#include "CYD2432S022CConstants.h"
#include <lvgl.h>
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
    lv_indev_t* getLvglIndev() override { return indev_; }

    // Additional getters (not overrides)
    int getWidth() const { return config_->width; }
    int getHeight() const { return config_->height; }

private:
    bool read_input(lv_indev_data_t* data);

    std::unique_ptr<Configuration> config_;
    lv_indev_t* indev_ = nullptr;
    lv_display_t* display_ = nullptr;  // Store display for rotation

    // Click filtering variables
    int32_t last_x_ = 0;
    int32_t last_y_ = 0;
    bool is_pressed_ = false;
    uint32_t touch_start_time_ = 0;  // Timestamp of touch start (in milliseconds)
};
