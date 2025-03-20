#pragma once

#include <Tactility/hal/touch/TouchDevice.h>
#include <memory>
#include "CYD2432S022CConstants.h"

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

    void init() override {}
    bool read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) override;
    int get_width() const override { return config_->width; }
    int get_height() const override { return config_->height; }

private:
    std::unique_ptr<Configuration> config_;
};
