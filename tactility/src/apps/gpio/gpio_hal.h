#pragma once

#ifdef ESP_PLATFORM
#include "driver/gpio.h"
#define GPIO_NUM_MIN GPIO_NUM_0
#else
#define GPIO_NUM_MIN 0
#define GPIO_NUM_MAX 50
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ESP_PLATFORM
int gpio_get_level(int gpio_num);
#endif

#ifdef __cplusplus
}
#endif