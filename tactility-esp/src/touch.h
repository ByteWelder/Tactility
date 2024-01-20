#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*CreateTouch)(esp_lcd_panel_io_handle_t* io_handle, esp_lcd_touch_handle_t* touch_handle);

typedef struct {
    char name[32];
    CreateTouch create_touch_device;
} TouchDriver;

typedef struct {
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_touch_handle_t touch_handle;
} TouchDevice;

/**
 * @param[in] driver
 * @return a newly allocated instance
 */
TouchDevice* tt_touch_alloc(TouchDriver* driver);

#ifdef __cplusplus
}
#endif
