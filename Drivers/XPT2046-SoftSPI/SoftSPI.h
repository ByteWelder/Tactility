#pragma once

#include <driver/gpio.h>

class SoftSPI {
public:
    SoftSPI(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sck);
    void begin();
    uint8_t transfer(uint8_t data);
    void cs_low();
    void cs_high();

private:
    gpio_num_t miso_;
    gpio_num_t mosi_;
    gpio_num_t sck_;
};
