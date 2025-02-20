#pragma once

#include <driver/ledc.h>
#include <driver/gpio.h>

namespace driver::pwmbacklight {

bool init(gpio_num_t pin, uint32_t frequencyHz = 40000, ledc_timer_t timer = LEDC_TIMER_0, ledc_channel_t channel = LEDC_CHANNEL_0);

bool setBacklightDuty(uint8_t duty);

}
