#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_display_t* tdeck_display_init();

bool tdeck_backlight_init();

void tdeck_backlight_set(uint8_t duty);

#ifdef __cplusplus
}
#endif