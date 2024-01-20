#pragma once

#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t display_handle;
    uint16_t horizontal_resolution;
    uint16_t vertical_resolution;
    uint16_t draw_buffer_height;
    uint16_t bits_per_pixel;
    bool double_buffering;
    bool mirror_x;
    bool mirror_y;
    bool swap_xy;
    bool monochrome;
} DisplayDevice;

typedef bool (*CreateDisplay)(DisplayDevice* display);

typedef struct {
    char name[32];
    CreateDisplay create_display_device;
} DisplayDriver;

/**
 * @param[in] driver
 * @return allocated display object
 */
DisplayDevice* tt_display_device_alloc(DisplayDriver* driver);

#ifdef __cplusplus
}
#endif
