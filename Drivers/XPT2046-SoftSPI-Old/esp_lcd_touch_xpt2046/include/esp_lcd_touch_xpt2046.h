#pragma once

#include <esp_lcd_touch.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LCD_TOUCH_SOFTSPI_CLOCK_HZ (500 * 1000)

// Extend ESP-IDF's config with calibration and orientation
typedef struct {
    esp_lcd_touch_config_t base;
    uint16_t x_min_raw;
    uint16_t x_max_raw;
    uint16_t y_min_raw;
    uint16_t y_max_raw;
    bool swap_xy;
    bool mirror_x;
    bool mirror_y;
} esp_lcd_touch_xpt2046_config_t;

#ifdef __cplusplus
}
// Forward declaration only; full definition is in XPT2046-SoftSPI.h
class XPT2046_SoftSPI;
#endif
