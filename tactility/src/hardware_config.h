#pragma once

#include "tactility_core.h"
#include "sdcard.h"

typedef bool (*Bootstrap)();
typedef bool (*InitLvgl)();

typedef struct {
    // Optional bootstrapping method (e.g. to turn peripherals on)
    const Bootstrap _Nullable bootstrap;
    const InitLvgl init_lvgl;
    const SdCard* _Nullable sdcard;
} HardwareConfig;
