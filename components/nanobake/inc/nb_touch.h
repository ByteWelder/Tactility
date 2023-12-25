#ifndef NANOBAKE_NB_TOUCH_H
#define NANOBAKE_NB_TOUCH_H

#include "esp_lcd_touch.h"
#include <esp_lcd_panel_io.h>

typedef struct nb_touch_driver nb_touch_driver_t;

struct nb_touch_driver {
    const char name[32];
    esp_err_t (*init_io)(esp_lcd_panel_io_handle_t* io_handle);
    esp_err_t (*create_touch)(esp_lcd_panel_io_handle_t io_handle, esp_lcd_touch_handle_t* touch_handle);
};

typedef struct nb_touch nb_touch_t;

struct nb_touch {
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_touch_handle_t touch_handle;
};

esp_err_t nb_touch_create(nb_touch_driver_t driver, nb_touch_t* touch);

#endif // NANOBAKE_NB_TOUCH_H
