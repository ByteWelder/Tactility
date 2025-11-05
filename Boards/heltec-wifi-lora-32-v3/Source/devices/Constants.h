#pragma once

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <driver/spi_common.h>

// Display
constexpr auto DISPLAY_I2C_ADDRESS = 0x3C;
constexpr auto DISPLAY_I2C_SPEED = 200000;
constexpr auto DISPLAY_I2C_PORT = I2C_NUM_0;
constexpr auto DISPLAY_PIN_SDA = GPIO_NUM_17;
constexpr auto DISPLAY_PIN_SCL = GPIO_NUM_18;
constexpr auto DISPLAY_PIN_RST = GPIO_NUM_21;
constexpr auto DISPLAY_HORIZONTAL_RESOLUTION = 128;
constexpr auto DISPLAY_VERTICAL_RESOLUTION = 64;
constexpr auto DISPLAY_PIN_POWER = GPIO_NUM_36;
