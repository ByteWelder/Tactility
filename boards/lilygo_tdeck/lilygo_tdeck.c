#include "lilygo_tdeck.h"
#include <stdbool.h>

bool tdeck_bootstrap();
bool tdeck_init_lvgl();

extern SdCard tdeck_sdcard;

const HardwareConfig lilygo_tdeck = {
    .bootstrap = &tdeck_bootstrap,
    .init_lvgl = &tdeck_init_lvgl,
    .sdcard = &tdeck_sdcard
};
