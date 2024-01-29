#pragma once

#include "tactility_core.h"
#include "sdcard.h"

typedef bool (*Bootstrap)();
typedef bool (*InitLvgl)();
typedef bool (*InitLvgl)();

typedef void (*SetBacklight)(uint8_t);
typedef struct {
    SetBacklight set_backlight;
} Display;

typedef struct {
    /**
     * Optional bootstrapping method (e.g. to turn peripherals on)
     * This is called after Tactility core init and before any other inits in the HardwareConfig.
     * */
    const Bootstrap _Nullable bootstrap;

    /**
     * Initializes LVGL with all relevant hardware.
     * This includes the display and optional pointer devices (such as touch) or a keyboard.
     */
    const InitLvgl init_lvgl;

    /**
     * An interface for display features such as setting the backlight.
     */
    const Display display;

    /**
     * An optional SD card interface.
     */
    const SdCard* _Nullable sdcard;
} HardwareConfig;
