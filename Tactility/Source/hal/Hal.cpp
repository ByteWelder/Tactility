#include "Tactility/Tactility.h"
#include "Tactility/hal/Configuration.h"
#include "Tactility/hal/Device.h"
#include "Tactility/hal/gps/GpsInit.h"
#include "Tactility/hal/i2c/I2cInit.h"
#include "Tactility/hal/power/PowerDevice.h"
#include "Tactility/hal/spi/SpiInit.h"
#include "Tactility/hal/uart/UartInit.h"

#include <Tactility/hal/display/DisplayDevice.h>
#include <Tactility/hal/sdcard/SdCardMounting.h>
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

    if (configuration.createDisplay != nullptr) {
        auto display = configuration.createDisplay();
        if (display != nullptr) {
            registerDevice(display);
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

static void startDisplays() {
    TT_LOG_I(TAG, "Start displays");
    auto displays = hal::findDevices<display::DisplayDevice>(Device::Type::Display);
    for (auto& display : displays) {
        if (!display->start()) {
            TT_LOG_E(TAG, "Display start failed");
        } else {
            TT_LOG_I(TAG, "Started %s", display->getName().c_str());

            if (display->supportsBacklightDuty()) {
                display->setBacklightDuty(0);
            }

            auto touch = display->getTouchDevice();
            if (touch != nullptr && !touch->start()) {
                TT_LOG_E(TAG, "Touch start failed");
            } else {
                TT_LOG_I(TAG, "Started %s", touch->getName().c_str());
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

    sdcard::mountAll(); // Warning: This needs to happen BEFORE displays are initialized on the SPI bus

    startDisplays(); // Warning: SPI displays need to start after SPI SD cards are mounted

    kernel::publishSystemEvent(kernel::SystemEvent::BootInitHalEnd);
}

const Configuration* getConfiguration() {
    return tt::getConfiguration()->hardware;
}

} // namespace
