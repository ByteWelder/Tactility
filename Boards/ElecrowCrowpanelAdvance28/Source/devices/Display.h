#pragma once

#include <Tactility/hal/display/DisplayDevice.h>

#define CROWPANEL_LCD_SPI_HOST SPI2_HOST
#define CROWPANEL_LCD_PIN_CS GPIO_NUM_40
#define CROWPANEL_LCD_PIN_DC GPIO_NUM_41 // RS
#define CROWPANEL_LCD_HORIZONTAL_RESOLUTION 320
#define CROWPANEL_LCD_VERTICAL_RESOLUTION 240
#define CROWPANEL_LCD_SPI_TRANSFER_HEIGHT (CROWPANEL_LCD_VERTICAL_RESOLUTION / 10)

std::shared_ptr<tt::hal::display::DisplayDevice> createDisplay();
