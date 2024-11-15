#pragma once

#include <cstdint>
#include <src/display/lv_display.h>

void display_preferences_set_backlight_duty(uint8_t value);
uint8_t display_preferences_get_backlight_duty();
void display_preferences_set_rotation(lv_display_rotation_t rotation);
lv_display_rotation_t display_preferences_get_rotation();
