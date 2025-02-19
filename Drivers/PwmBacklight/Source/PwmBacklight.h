#pragma once

#include <driver/gpio.h>

namespace driver::pwmbacklight {

bool init(gpio_num_t pin);

void setBacklightDuty(uint8_t duty);

}
