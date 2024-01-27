#include "lilygo_tdeck.h"
#include <stdbool.h>

bool tdeck_bootstrap();
bool tdeck_init_lvgl();

const HardwareConfig lilygo_tdeck = {
    .bootstrap = &tdeck_bootstrap,
    .init_lvgl = &tdeck_init_lvgl
};
