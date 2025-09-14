#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <memory>

#define LCD_SPI_HOST SPI2_HOST
#define LCD_PIN_CS GPIO_NUM_37
#define LCD_PIN_DC GPIO_NUM_34 // RS
#define LCD_PIN_RESET GPIO_NUM_33
#define LCD_HORIZONTAL_RESOLUTION 240
#define LCD_VERTICAL_RESOLUTION 135
#define LCD_DRAW_BUFFER_HEIGHT (LCD_VERTICAL_RESOLUTION / 10)
#define LCD_DRAW_BUFFER_SIZE (LCD_HORIZONTAL_RESOLUTION * LCD_DRAW_BUFFER_HEIGHT)

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
