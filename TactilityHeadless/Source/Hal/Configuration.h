#pragma once

#include "Power.h"
#include "Sdcard.h"
#include "I2c/I2c.h"
#include "TactilityCore.h"

namespace tt::hal {

typedef bool (*InitPower)();
typedef bool (*InitHardware)();
typedef bool (*InitLvgl)();

typedef void (*SetBacklightDuty)(uint8_t);
typedef struct {
    /** Set backlight duty */
    _Nullable SetBacklightDuty setBacklightDuty;
} Display;

typedef struct {
    /**
     * Called before I2C/SPI/etc is initialized.
     * Used for powering on the peripherals manually.
     */
    const InitPower _Nullable initPower = nullptr;

    /**
     * Called after I2C/SPI/etc is initialized.
     * This can be used to communicate with built-in peripherals such as an I2C keyboard.
     */
    const InitHardware _Nullable initHardware = nullptr;

    /**
     * Create and initialize all LVGL devices. (e.g. display, touch, keyboard)
     */
    const InitLvgl initLvgl;

    /**
     * Display HAL functionality.
     */
    const Display display;

    /**
     * An optional SD card interface.
     */
    const sdcard::SdCard* _Nullable sdcard;

    /**
     * An optional power interface for battery or other power delivery.
     */
    const Power* _Nullable power;

    /**
     * A list of i2c devices (can be empty, but preferably accurately represents the device capabilities)
     */
    const std::vector<i2c::Configuration> i2c;
} Configuration;

} // namespace
