#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_common.h"

// Display
#define HELTEC_LCD_I2C_ADDRESS 0x3C
#define HELTEC_LCD_I2C_SPEED 200000
#define HELTEC_LCD_I2C_PORT I2C_NUM_0
#define HELTEC_LCD_PIN_SDA GPIO_NUM_17
#define HELTEC_LCD_PIN_SCL GPIO_NUM_18
#define HELTEC_LCD_PIN_RST GPIO_NUM_21
#define HELTEC_LCD_HORIZONTAL_RESOLUTION 128
#define HELTEC_LCD_VERTICAL_RESOLUTION 64
#define HELTEC_LCD_PIN_POWER GPIO_NUM_36
