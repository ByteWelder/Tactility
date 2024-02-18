/**
* The WaveShare S3 Touch uses a panel with the ST7262 display driver.
*/
#pragma once

#define WAVESHARE_LCD_HOR_RES 800
#define WAVESHARE_LCD_VER_RES 480

#define WAVESHARE_LCD_PIXEL_CLOCK_HZ (12 * 1000 * 1000) // NOTE: original was 14MHz, but we had to slow it down with PSRAM frame buffer
#define WAVESHARE_LCD_PIN_NUM_HSYNC 46
#define WAVESHARE_LCD_PIN_NUM_VSYNC 3
#define WAVESHARE_LCD_PIN_NUM_DE 5
#define WAVESHARE_LCD_PIN_NUM_PCLK 7
#define WAVESHARE_LCD_PIN_NUM_DATA0 14  // B3
#define WAVESHARE_LCD_PIN_NUM_DATA1 38  // B4
#define WAVESHARE_LCD_PIN_NUM_DATA2 18  // B5
#define WAVESHARE_LCD_PIN_NUM_DATA3 17  // B6
#define WAVESHARE_LCD_PIN_NUM_DATA4 10  // B7
#define WAVESHARE_LCD_PIN_NUM_DATA5 39  // G2
#define WAVESHARE_LCD_PIN_NUM_DATA6 0   // G3
#define WAVESHARE_LCD_PIN_NUM_DATA7 45  // G4
#define WAVESHARE_LCD_PIN_NUM_DATA8 48  // G5
#define WAVESHARE_LCD_PIN_NUM_DATA9 47  // G6
#define WAVESHARE_LCD_PIN_NUM_DATA10 21 // G7
#define WAVESHARE_LCD_PIN_NUM_DATA11 1  // R3
#define WAVESHARE_LCD_PIN_NUM_DATA12 2  // R4
#define WAVESHARE_LCD_PIN_NUM_DATA13 42 // R5
#define WAVESHARE_LCD_PIN_NUM_DATA14 41 // R6
#define WAVESHARE_LCD_PIN_NUM_DATA15 40 // R7
#define WAVESHARE_LCD_PIN_NUM_DISP_EN (-1)
#define WAVESHARE_LCD_BUFFER_HEIGHT (WAVESHARE_LCD_VER_RES / 3) // How many rows of pixels to buffer - 1/3rd is about 1MB

#define WAVESHARE_LCD_USE_DOUBLE_FB true // Performance boost at the cost of about extra PSRAM(SPIRAM)

#define WAVESHARE_LVGL_TICK_PERIOD_MS 2 // TODO: Setting it to 5 causes a crash - why?
#define LVGL_MAX_SLEEP 500

#define WAVESHARE_TOUCH_I2C_PORT 0
