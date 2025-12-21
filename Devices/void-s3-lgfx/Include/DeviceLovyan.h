#pragma once

#include "LGFX.hpp"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

LGFX &getLovyan();

/**
 * Initialize hardware (LCD, touch). This does not call lv_init().
 * Returns true on success.
 */
bool lovyan_hw_init();

/**
 * Create an LVGL display and input device using LovyanGFX.
 * lvgl must be initialized before calling this.
 * Returns true on success.
 */
bool lovyan_lvgl_bind(void);

/* Accessors */
lv_display_t* lovyan_get_display(void);
lv_indev_t* lovyan_get_indev(void);
bool lovyan_lvgl_unbind(void);
void lovyan_set_backlight(uint8_t duty);

#ifdef __cplusplus
}
#endif
