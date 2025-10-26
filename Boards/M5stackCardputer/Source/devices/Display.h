#pragma once

#include <Tactility/hal/display/DisplayDevice.h>
#include <memory>
#include <driver/gpio.h>
#include <driver/spi_common.h>

constexpr auto LCD_SPI_HOST = SPI2_HOST;
constexpr auto LCD_PIN_CS = GPIO_NUM_37;
constexpr auto LCD_PIN_DC = GPIO_NUM_34; // RS
constexpr auto LCD_PIN_RESET = GPIO_NUM_33;
constexpr auto LCD_HORIZONTAL_RESOLUTION = 240;
constexpr auto LCD_VERTICAL_RESOLUTION = 135;
constexpr auto LCD_BUFFER_HEIGHT = LCD_VERTICAL_RESOLUTION / 10;
constexpr auto LCD_BUFFER_SIZE = LCD_HORIZONTAL_RESOLUTION * LCD_BUFFER_HEIGHT;
constexpr auto LCD_SPI_TRANSFER_SIZE_LIMIT = LCD_BUFFER_SIZE * LV_COLOR_DEPTH / 8;

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
