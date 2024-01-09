#pragma once

#include "tactility.h"

#ifdef __cplusplus
extern "C" {
#endif

// Available for HardwareConfig customizations
void lilygo_tdeck_bootstrap();
DisplayDriver lilygo_tdeck_display_driver();
TouchDriver lilygo_tdeck_touch_driver();

extern const HardwareConfig lilygo_tdeck;

#ifdef __cplusplus
}
#endif
