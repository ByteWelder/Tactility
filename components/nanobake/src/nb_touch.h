#pragma once

#include "esp_lcd_touch.h"
#include <esp_lcd_panel_io.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*CreateTouch)(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle);

typedef struct {
    char name[32];
    CreateTouch create_touch;
} NbTouchDriver;

typedef struct {
    esp_lcd_panel_io_handle_t _Nonnull io_handle;
    esp_lcd_touch_handle_t _Nonnull touch_handle;
} NbTouch;

/**
 * @param[in] driver
 * @return a newly allocated instance
 */
NbTouch _Nonnull* nb_touch_alloc(NbTouchDriver _Nonnull* driver);

#ifdef __cplusplus
}
#endif
