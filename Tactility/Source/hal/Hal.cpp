#include "Tactility/Tactility.h"
#include "Tactility/hal/Configuration.h"
#include "Tactility/hal/Device.h"
#include "Tactility/hal/gps/GpsInit.h"
#include "Tactility/hal/i2c/I2cInit.h"
#include "Tactility/hal/power/PowerDevice.h"
#include "Tactility/hal/spi/SpiInit.h"
#include "Tactility/hal/uart/UartInit.h"

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/touch/TouchDevice.h>
#include <Tactility/kernel/SystemEvents.h>

namespace tt::hal {

constexpr auto* TAG = "Hal";

void registerDevices(const Configuration& configuration) {
    TT_LOG_I(TAG, "Registering devices");

    if (configuration.sdcard != nullptr) {
        registerDevice(configuration.sdcard);
    }

    if (configuration.power != nullptr) {
        std::shared_ptr<power::PowerDevice> power = configuration.power();
        registerDevice(power);
    }

    if (configuration.createKeyboard) {
        auto keyboard = configuration.createKeyboard();
        if (keyboard != nullptr) {
            registerDevice(std::reinterpret_pointer_cast<Device>(keyboard));
        }
    }

    auto devices = configuration.createDevices();
    for (auto& device : devices) {
        registerDevice(device);

        // Register attached devices
        if (device->getType() == Device::Type::Display) {
            const auto display = std::static_pointer_cast<display::DisplayDevice>(device);
            assert(display != nullptr);
            const std::shared_ptr<Device> touch = display->getTouchDevice();
            if (touch != nullptr) {
                registerDevice(touch);
            }
        }
    }
}

void init(const Configuration& configuration) {
    kernel::publishSystemEvent(kernel::SystemEvent::BootInitHalBegin);

    kernel::publishSystemEvent(kernel::SystemEvent::BootInitI2cBegin);
    tt_check(i2c::init(configuration.i2c), "I2C init failed");
    kernel::publishSystemEvent(kernel::SystemEvent::BootInitI2cEnd);

    kernel::publishSystemEvent(kernel::SystemEvent::BootInitSpiBegin);
    tt_check(spi::init(configuration.spi), "SPI init failed");
    kernel::publishSystemEvent(kernel::SystemEvent::BootInitSpiEnd);

    kernel::publishSystemEvent(kernel::SystemEvent::BootInitUartBegin);
    tt_check(uart::init(configuration.uart), "UART init failed");
    kernel::publishSystemEvent(kernel::SystemEvent::BootInitUartEnd);

    if (configuration.initBoot != nullptr) {
        TT_LOG_I(TAG, "Init power");
        tt_check(configuration.initBoot(), "Init power failed");
    }

    registerDevices(configuration);

    kernel::publishSystemEvent(kernel::SystemEvent::BootInitHalEnd);
}

const Configuration* getConfiguration() {
    return tt::getConfiguration()->hardware;
}

} // namespace
