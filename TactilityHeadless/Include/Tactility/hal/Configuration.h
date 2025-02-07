#pragma once

#include "./Power.h"
#include "./SdCard.h"
#include "./i2c/I2c.h"
#include "Tactility/hal/spi/Spi.h"

namespace tt::hal {

typedef bool (*InitBoot)();
typedef bool (*InitHardware)();
typedef bool (*InitLvgl)();

class Display;
class Keyboard;
typedef std::shared_ptr<Display> (*CreateDisplay)();
typedef std::shared_ptr<Keyboard> (*CreateKeyboard)();
typedef std::shared_ptr<Power> (*CreatePower)();

struct Configuration {
    /**
     * Called before I2C/SPI/etc is initialized.
     * Used for powering on the peripherals manually.
     */
    const InitBoot _Nullable initBoot = nullptr;

    /** Create and initialize all LVGL devices. (e.g. display, touch, keyboard) */
    const InitLvgl _Nullable initLvgl = nullptr;

    /** Display HAL functionality. */
    const CreateDisplay _Nullable createDisplay = nullptr;

    /** Display HAL functionality. */
    const CreateKeyboard _Nullable createKeyboard = nullptr;

    /** An optional SD card interface. */
    const std::shared_ptr<SdCard> _Nullable sdcard = nullptr;

    /** An optional power interface for battery or other power delivery. */
    const CreatePower _Nullable power = nullptr;

    /** A list of I2C interfaces */
    const std::vector<i2c::Configuration> i2c = {};

    /** A list of SPI interfaces */
    const std::vector<spi::Configuration> spi = {};
};

} // namespace
