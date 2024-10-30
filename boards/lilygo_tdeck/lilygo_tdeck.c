#include "lilygo_tdeck.h"
#include "display_i.h"
#include <stdbool.h>

bool tdeck_bootstrap();
bool tdeck_init_lvgl();

extern const SdCard tdeck_sdcard;

const HardwareConfig lilygo_tdeck = {
    .bootstrap = &tdeck_bootstrap,
    .display = {
        .set_backlight_duty = &tdeck_backlight_set
    },
    .init_graphics = &tdeck_init_lvgl,
    .sdcard = &tdeck_sdcard,
    .power = NULL
};
