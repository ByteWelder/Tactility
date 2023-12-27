#pragma once

#include "esp_lcd_touch.h"
#include <esp_lcd_panel_io.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nb_touch_driver nb_touch_driver_t;

struct nb_touch_driver {
    char name[32];
    bool (*create_touch)(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle);
};

typedef struct nb_touch nb_touch_t;

struct nb_touch {
    esp_lcd_panel_io_handle_t _Nonnull io_handle;
    esp_lcd_touch_handle_t _Nonnull touch_handle;
};

/**
 * @param[in] driver
 * @return a newly allocated instance
 */
nb_touch_t _Nonnull* nb_touch_alloc(nb_touch_driver_t _Nonnull* driver);

#ifdef __cplusplus
}
#endif
