#pragma once

// Display
#define CYD_DISPLAY_SPI_HOST SPI2_HOST
#define CYD_DISPLAY_PIN_CS GPIO_NUM_15
#define CYD_DISPLAY_PIN_DC GPIO_NUM_2
#define CYD_DISPLAY_HORIZONTAL_RESOLUTION 240
#define CYD_DISPLAY_VERTICAL_RESOLUTION 320
#define CYD_DISPLAY_DRAW_BUFFER_HEIGHT (CYD_DISPLAY_VERTICAL_RESOLUTION / 10)
#define CYD_DISPLAY_DRAW_BUFFER_SIZE (CYD_DISPLAY_HORIZONTAL_RESOLUTION * CYD_DISPLAY_DRAW_BUFFER_HEIGHT)

// Touch (Software SPI)
#define CYD_TOUCH_MISO_PIN GPIO_NUM_39
#define CYD_TOUCH_MOSI_PIN GPIO_NUM_32
#define CYD_TOUCH_SCK_PIN GPIO_NUM_25
#define CYD_TOUCH_CS_PIN GPIO_NUM_33
#define CYD_TOUCH_IRQ_PIN GPIO_NUM_NC

// SD Card
#define CYD_SDCARD_SPI_HOST SPI3_HOST
#define CYD_SDCARD_PIN_CS GPIO_NUM_5


// Backlight
#define CYD_BACKLIGHT_PIN GPIO_NUM_21
