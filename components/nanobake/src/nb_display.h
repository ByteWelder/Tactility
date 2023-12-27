#pragma once

#include <esp_lcd_panel_io.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nb_display nb_display_t;

struct nb_display {
    uint16_t horizontal_resolution;
    uint16_t vertical_resolution;
    uint16_t draw_buffer_height;
    uint16_t bits_per_pixel;
    esp_lcd_panel_io_handle_t _Nonnull io_handle;
    esp_lcd_panel_handle_t _Nonnull display_handle;
    bool mirror_x;
    bool mirror_y;
    bool swap_xy;
};

typedef struct nb_display_driver nb_display_driver_t;

struct nb_display_driver {
    char name[32];
    bool (*create_display)(nb_display_t* display);
};

/**
 * @param[in] driver
 * @return allocated display object
 */
nb_display_t _Nonnull* nb_display_alloc(nb_display_driver_t _Nonnull* driver);

#ifdef __cplusplus
}
#endif
