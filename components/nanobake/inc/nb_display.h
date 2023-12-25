#ifndef NANOBAKE_NB_DISPLAY_H
#define NANOBAKE_NB_DISPLAY_H

#include <esp_lcd_panel_io.h>

typedef struct nb_display nb_display_t;

struct nb_display {
    bool io_initialized;
    uint16_t horizontal_resolution;
    uint16_t vertical_resolution;
    uint16_t draw_buffer_height;
    uint16_t bits_per_pixel;
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t display_handle;
};

typedef struct nb_display_driver nb_display_driver_t;

struct nb_display_driver {
    const char name[32];
    esp_err_t (*create_display)(nb_display_t* display);
};

extern esp_err_t nb_display_create(nb_display_driver_t driver, nb_display_t* display);

#endif // NANOBAKE_NB_DISPLAY_H
