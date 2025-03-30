#pragma once

#ifdef ESP_PLATFORM
#include <driver/gpio.h>
#else
typedef unsigned int gpio_num_t;
#endif