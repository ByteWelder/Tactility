#pragma once

#include "Power.h"
#include "hal/i2c/I2c.h"
#include "SdCard.h"

namespace tt::hal {

typedef bool (*InitBoot)();
typedef bool (*InitHardware)();
typedef bool (*InitLvgl)();

class Display;
class Keyboard;
typedef Display* (*CreateDisplay)();
typedef Keyboard* (*CreateKeyboard)();
typedef std::shared_ptr<Power> (*CreatePower)();

struct Configuration {
    /**
     * Called before I2C/SPI/etc is initialized.
     * Used for powering on the peripherals manually.
     */
    const InitBoot _Nullable initBoot = nullptr;

    /**
     * Called after I2C/SPI/etc is initialized.
     * This can be used to communicate with built-in peripherals such as an I2C keyboard.
     */
    const InitHardware _Nullable initHardware = nullptr;

    /**
     * Create and initialize all LVGL devices. (e.g. display, touch, keyboard)
     */
    const InitLvgl _Nullable initLvgl = nullptr;

    /**
     * Display HAL functionality.
     */
    const CreateDisplay _Nullable createDisplay = nullptr;

    /**
     * Display HAL functionality.
     */
    const CreateKeyboard _Nullable createKeyboard = nullptr;

    /**
     * An optional SD card interface.
     */
    const std::shared_ptr<SdCard> _Nullable sdcard = nullptr;

    /**
     * An optional power interface for battery or other power delivery.
     */
    const CreatePower _Nullable power = nullptr;

    /**
     * A list of i2c interfaces
     */
    const std::vector<i2c::Configuration> i2c = {};
};

} // namespace
