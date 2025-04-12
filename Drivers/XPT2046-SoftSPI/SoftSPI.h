#pragma once

#include <driver/gpio.h>

class SoftSPI {
public:
    SoftSPI(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sck);
    void begin();
    uint8_t transfer(uint8_t data);
    void cs_low() { gpio_set_level(cs_pin_, 0); }
    void cs_high() { gpio_set_level(cs_pin_, 1); }

private:
    gpio_num_t miso_;
    gpio_num_t mosi_;
    gpio_num_t sck_;
    gpio_num_t cs_pin_;
};
