#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool twodotfour_backlight_init();
void twodotfour_backlight_set(uint8_t duty);

#ifdef __cplusplus
}
#endif