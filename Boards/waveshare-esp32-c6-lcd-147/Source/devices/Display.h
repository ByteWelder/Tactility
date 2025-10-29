#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <memory>

// Display SPI configuration
constexpr auto LCD_SPI_HOST = SPI2_HOST;
constexpr auto LCD_PIN_CS = GPIO_NUM_14;
constexpr auto LCD_PIN_DC = GPIO_NUM_15;
constexpr auto LCD_PIN_RESET = GPIO_NUM_21;
constexpr auto LCD_PIXEL_CLOCK_HZ = 12'000'000;  // 12 MHz as in demo

// Display panel configuration
constexpr auto LCD_HORIZONTAL_RESOLUTION = 172;
constexpr auto LCD_VERTICAL_RESOLUTION = 320;
constexpr auto LCD_GAP_X = 34;  // X offset for 1.47" display
constexpr auto LCD_GAP_Y = 0;

// Display backlight
constexpr auto LCD_PIN_BACKLIGHT = GPIO_NUM_22;

// SPI bus configuration (shared with SD card)
constexpr auto LCD_PIN_MOSI = GPIO_NUM_6;
constexpr auto LCD_PIN_MISO = GPIO_NUM_5;
constexpr auto LCD_PIN_SCLK = GPIO_NUM_7;
constexpr auto LCD_BUFFER_HEIGHT = LCD_VERTICAL_RESOLUTION / 10;
constexpr auto LCD_BUFFER_SIZE = LCD_HORIZONTAL_RESOLUTION * LCD_BUFFER_HEIGHT;
constexpr auto LCD_SPI_TRANSFER_SIZE_LIMIT = LCD_BUFFER_SIZE * LV_COLOR_DEPTH / 8;

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
