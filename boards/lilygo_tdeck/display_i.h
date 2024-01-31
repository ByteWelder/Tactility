#pragma once

#include "hal/lv_hal_disp.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

lv_disp_t* tdeck_display_init();

bool tdeck_backlight_init();

void tdeck_backlight_set(uint8_t duty);

#ifdef __cplusplus
}
#endif