#pragma once

#include "./SdCard.h"
#include "./i2c/I2c.h"
#include "Tactility/hal/spi/Spi.h"
#include "Tactility/hal/uart/Uart.h"

namespace tt::hal {

typedef bool (*InitBoot)();

class Display;
class Keyboard;
class Power;
typedef std::shared_ptr<Display> (*CreateDisplay)();
typedef std::shared_ptr<Keyboard> (*CreateKeyboard)();
typedef std::shared_ptr<Power> (*CreatePower)();

enum class LvglInit {
    Default,
    None
};

struct Configuration {
    /**
     * Called before I2C/SPI/etc is initialized.
     * Used for powering on the peripherals manually.
     */
    const InitBoot _Nullable initBoot = nullptr;

    /** Init behaviour: default (esp_lvgl_port for ESP32, nothing for PC) or None (nothing on any platform). Only used in Tactility, not in TactilityHeadless. */
    const LvglInit lvglInit = LvglInit::Default;

    /** Display HAL functionality. */
    const CreateDisplay _Nullable createDisplay = nullptr;

    /** Keyboard HAL functionality. */
    const CreateKeyboard _Nullable createKeyboard = nullptr;

    /** An optional SD card interface. */
    const std::shared_ptr<SdCard> _Nullable sdcard = nullptr;

    /** An optional power interface for battery or other power delivery. */
    const CreatePower _Nullable power = nullptr;

    /** A list of I2C interfaces */
    const std::vector<i2c::Configuration> i2c = {};

    /** A list of SPI interfaces */
    const std::vector<spi::Configuration> spi = {};

    /** A list of UART interfaces */
    const std::vector<uart::Configuration> uart = {};
};

} // namespace
