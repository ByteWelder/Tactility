#pragma once

#include <Tactility/hal/display/DisplayDevice.h>

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();

constexpr auto LCD_HORIZONTAL_RESOLUTION = 240;
constexpr auto LCD_VERTICAL_RESOLUTION = 240;
constexpr auto LCD_BUFFER_HEIGHT = LCD_VERTICAL_RESOLUTION / 3;
constexpr auto LCD_BUFFER_SIZE = LCD_HORIZONTAL_RESOLUTION * LCD_BUFFER_HEIGHT;
constexpr auto LCD_SPI_TRANSFER_SIZE_LIMIT = LCD_BUFFER_SIZE * LV_COLOR_DEPTH / 8;
