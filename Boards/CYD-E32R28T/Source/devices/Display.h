#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <memory>

// Display
constexpr auto LCD_SPI_HOST = SPI2_HOST;
constexpr auto LCD_PIN_CS = GPIO_NUM_15;
constexpr auto LCD_PIN_DC = GPIO_NUM_2;
constexpr auto LCD_HORIZONTAL_RESOLUTION = 240;
constexpr auto LCD_VERTICAL_RESOLUTION = 320;
constexpr auto LCD_BUFFER_HEIGHT = (LCD_VERTICAL_RESOLUTION / 10);
constexpr auto LCD_BUFFER_SIZE = (LCD_HORIZONTAL_RESOLUTION * LCD_BUFFER_HEIGHT);
constexpr auto LCD_SPI_TRANSFER_SIZE_LIMIT = LCD_BUFFER_SIZE * LV_COLOR_DEPTH / 8;

// Touch (Software SPI)
constexpr auto TOUCH_MISO_PIN = GPIO_NUM_39;
constexpr auto TOUCH_MOSI_PIN = GPIO_NUM_32;
constexpr auto TOUCH_SCK_PIN = GPIO_NUM_25;
constexpr auto TOUCH_CS_PIN = GPIO_NUM_33;
constexpr auto TOUCH_IRQ_PIN = GPIO_NUM_36;

// Backlight
constexpr auto LCD_BACKLIGHT_PIN = GPIO_NUM_21;

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
