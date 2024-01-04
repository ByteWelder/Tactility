#include "lilygo_tdeck.h"

void lilygo_tdeck_bootstrap();
DisplayDriver lilygo_tdeck_display_driver();
TouchDriver lilygo_tdeck_touch_driver();

const HardwareConfig lilygo_tdeck = {
    .bootstrap = &lilygo_tdeck_bootstrap,
    .display_driver = &lilygo_tdeck_display_driver,
    .touch_driver = &lilygo_tdeck_touch_driver
};
