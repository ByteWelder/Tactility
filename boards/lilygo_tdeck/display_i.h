#pragma once

#include "lvgl.h"

lv_display_t* tdeck_display_init();

bool tdeck_backlight_init();

void tdeck_backlight_set(uint8_t duty);