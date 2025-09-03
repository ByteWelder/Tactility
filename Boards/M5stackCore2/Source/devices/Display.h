#pragma once

#include "Tactility/hal/display/DisplayDevice.h"
#include <memory>

#define CORE2_LCD_SPI_HOST SPI2_HOST
#define CORE2_LCD_PIN_CS GPIO_NUM_5
#define CORE2_LCD_PIN_DC GPIO_NUM_15
#define CORE2_LCD_HORIZONTAL_RESOLUTION 320
#define CORE2_LCD_VERTICAL_RESOLUTION 240
#define CORE2_LCD_DRAW_BUFFER_HEIGHT (CORE2_LCD_VERTICAL_RESOLUTION / 10)
#define CORE2_LCD_DRAW_BUFFER_SIZE (CORE2_LCD_HORIZONTAL_RESOLUTION * CORE2_LCD_DRAW_BUFFER_HEIGHT)

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
