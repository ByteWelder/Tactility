#pragma once

#include <Tactility/hal/sdcard/SdCardDevice.h>
#include <Tactility/hal/spi/Spi.h>
#include <Tactility/hal/uart/Uart.h>
#include "i2c/I2c.h"

namespace tt::hal {

typedef bool (*InitBoot)();

typedef std::vector<std::shared_ptr<Device>> DeviceVector;

typedef std::shared_ptr<Device> (*CreateDevice)();

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

    std::function<DeviceVector()> createDevices = [] { return std::vector<std::shared_ptr<Device>>(); };

    /** A list of I2C interface configurations */
    const std::vector<i2c::Configuration> i2c = {};

    /** A list of SPI interface configurations */
    const std::vector<spi::Configuration> spi = {};

    /** A list of UART interface configurations */
    const std::vector<uart::Configuration> uart = {};
};

} // namespace
