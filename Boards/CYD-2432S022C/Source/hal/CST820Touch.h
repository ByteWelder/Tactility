#pragma once

#include <Tactility/hal/touch/TouchDevice.h>
#include <memory>

std::shared_ptr<tt::hal::touch::TouchDevice> create_cst820_touch();

class Cst820Touch : public tt::hal::touch::TouchDevice {
public:
    struct Configuration {
        i2c_port_t i2c_port;
        int width;
        int height;
    };

    Cst820Touch(std::unique_ptr<Configuration> config);
    ~Cst820Touch() override = default;

    void init() override {}
    bool read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) override;
    int get_width() const override { return config_->width; }
    int get_height() const override { return config_->height; }

private:
    static constexpr uint8_t I2C_ADDR = 0x15;
    std::unique_ptr<Configuration> config_;
};
