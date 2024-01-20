#pragma once

#include "hardware.h"
#include "tactility.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef void (*Bootstrap)();
typedef TouchDriver (*CreateTouchDriver)();
typedef DisplayDriver (*CreateDisplayDriver)();

typedef struct {
    // Optional bootstrapping method (e.g. to turn peripherals on)
    const Bootstrap _Nullable bootstrap;
    // Required driver for display
    const CreateDisplayDriver display_driver;
    // Optional driver for touch input
    const CreateTouchDriver _Nullable touch_driver;
} HardwareConfig;


void tt_esp_init(const HardwareConfig* hardware_config);

#ifdef __cplusplus
}
#endif
