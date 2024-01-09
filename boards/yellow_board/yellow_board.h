#pragma once

#include "tactility.h"

#ifdef __cplusplus
extern "C" {
#endif

// Available for HardwareConfig customizations
DisplayDriver board_2432s024_create_display_driver();
TouchDriver board_2432s024_create_touch_driver();

// Capacitive touch version of the 2.4" yellow board
extern const HardwareConfig yellow_board_24inch_cap;

#ifdef __cplusplus
}
#endif
