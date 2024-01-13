#include "lilygo_tdeck.h"

const HardwareConfig lilygo_tdeck = {
    .bootstrap = &lilygo_tdeck_bootstrap,
    .display_driver = &lilygo_tdeck_display_driver,
    .touch_driver = &lilygo_tdeck_touch_driver
};
