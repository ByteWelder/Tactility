#include "lilygo_tdeck.h"
#include <stdbool.h>

bool lilygo_tdeck_bootstrap();
bool lilygo_init_lvgl();

const HardwareConfig lilygo_tdeck = {
    .bootstrap = &lilygo_tdeck_bootstrap,
    .init_lvgl = &lilygo_init_lvgl
};
