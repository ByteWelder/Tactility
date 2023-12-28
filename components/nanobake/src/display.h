#pragma once

#include <esp_lcd_panel_io.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    esp_lcd_panel_io_handle_t _Nonnull io_handle;
    esp_lcd_panel_handle_t _Nonnull display_handle;
    uint16_t horizontal_resolution;
    uint16_t vertical_resolution;
    uint16_t draw_buffer_height;
    uint16_t bits_per_pixel;
    bool mirror_x;
    bool mirror_y;
    bool swap_xy;
} DisplayDevice;

typedef bool (*CreateDisplay)(DisplayDevice* display);

typedef struct {
    char name[32];
    CreateDisplay create_display;
} DisplayDriver;

/**
 * @param[in] driver
 * @return allocated display object
 */
DisplayDevice _Nonnull* nb_display_alloc(DisplayDriver _Nonnull* driver);

#ifdef __cplusplus
}
#endif
