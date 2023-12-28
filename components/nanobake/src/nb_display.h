#pragma once

#include <esp_lcd_panel_io.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t horizontal_resolution;
    uint16_t vertical_resolution;
    uint16_t draw_buffer_height;
    uint16_t bits_per_pixel;
    esp_lcd_panel_io_handle_t _Nonnull io_handle;
    esp_lcd_panel_handle_t _Nonnull display_handle;
    bool mirror_x;
    bool mirror_y;
    bool swap_xy;
} NbDisplay;

typedef bool (*CreateDisplay)(NbDisplay* display);

typedef struct {
    char name[32];
    CreateDisplay create_display;
} NbDisplayDriver;

/**
 * @param[in] driver
 * @return allocated display object
 */
NbDisplay _Nonnull* nb_display_alloc(NbDisplayDriver _Nonnull* driver);

#ifdef __cplusplus
}
#endif
