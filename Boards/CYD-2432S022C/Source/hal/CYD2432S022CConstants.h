#pragma once

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/spi_common.h"

// Display (ST7789 with 8-bit parallel interface)
#define CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION 240
#define CYD_2432S022C_LCD_VERTICAL_RESOLUTION 320
#define CYD_2432S022C_LCD_DRAW_BUFFER_HEIGHT (CYD_2432S022C_LCD_VERTICAL_RESOLUTION / 20) // 16
#define CYD_2432S022C_LCD_DRAW_BUFFER_SIZE (CYD_2432S022C_LCD_HORIZONTAL_RESOLUTION * CYD_2432S022C_LCD_DRAW_BUFFER_HEIGHT) // 3840 pixels
#define CYD_2432S022C_LCD_PIN_BACKLIGHT GPIO_NUM_0
#define CYD_2432S022C_LCD_PCLK_HZ (10 * 1000 * 1000) // 10 MHz
#define CYD_2432S022C_LCD_TRANS_DESCRIPTOR_NUM 10

// GPIO pins for ST7789 8-bit parallel interface (from schematic)
#define CYD_2432S022C_LCD_PIN_D0 GPIO_NUM_15
#define CYD_2432S022C_LCD_PIN_D1 GPIO_NUM_13
#define CYD_2432S022C_LCD_PIN_D2 GPIO_NUM_12
#define CYD_2432S022C_LCD_PIN_D3 GPIO_NUM_14
#define CYD_2432S022C_LCD_PIN_D4 GPIO_NUM_27
#define CYD_2432S022C_LCD_PIN_D5 GPIO_NUM_25
#define CYD_2432S022C_LCD_PIN_D6 GPIO_NUM_33
#define CYD_2432S022C_LCD_PIN_D7 GPIO_NUM_32

#define CYD_2432S022C_LCD_PIN_WR GPIO_NUM_4
#define CYD_2432S022C_LCD_PIN_RD GPIO_NUM_2
#define CYD_2432S022C_LCD_PIN_DC GPIO_NUM_16
#define CYD_2432S022C_LCD_PIN_CS GPIO_NUM_17
#define CYD_2432S022C_LCD_PIN_RST GPIO_NUM_NC

// I2C for CST820 touch controller
#define CYD_2432S022C_TOUCH_I2C_PORT I2C_NUM_0
#define CYD_2432S022C_TOUCH_I2C_SDA GPIO_NUM_21
#define CYD_2432S022C_TOUCH_I2C_SCL GPIO_NUM_22
#define CYD_2432S022C_TOUCH_I2C_ADDRESS 0x15
#define CYD_2432S022C_TOUCH_I2C_SPEED 400000

// SPI for SD card
#define CYD_2432S022C_SDCARD_SPI_HOST SPI3_HOST
#define CYD_2432S022C_SDCARD_PIN_CS GPIO_NUM_5
#define CYD_2432S022C_SDCARD_PIN_MOSI GPIO_NUM_23
#define CYD_2432S022C_SDCARD_PIN_MISO GPIO_NUM_19
#define CYD_2432S022C_SDCARD_PIN_SCLK GPIO_NUM_18
#define CYD_2432S022C_SDCARD_SPI_MAX_TRANSFER_SIZE 8192

// Backlight PWM (LEDC)
#define CYD_2432S022C_LCD_BACKLIGHT_PWM_FREQ_HZ 44100
#define CYD_2432S022C_LCD_BACKLIGHT_LEDC_TIMER LEDC_TIMER_0
#define CYD_2432S022C_LCD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_0
#define CYD_2432S022C_LCD_BACKLIGHT_DUTY_RES LEDC_TIMER_8_BIT
