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
    TT_LOG_I(TAG, "Starting displays & touch");
    auto displays = hal::findDevices<display::DisplayDevice>(Device::Type::Display);
    for (auto& display : displays) {
        TT_LOG_I(TAG, "%s starting", display->getName().c_str());
        if (!display->start()) {
            TT_LOG_E(TAG, "%s start failed", display->getName().c_str());
        } else {
            TT_LOG_I(TAG, "%s started", display->getName().c_str());

            if (display->supportsBacklightDuty()) {
                TT_LOG_I(TAG, "Setting backlight");
                display->setBacklightDuty(0);
            }

            auto touch = display->getTouchDevice();
            if (touch != nullptr) {
                TT_LOG_I(TAG, "%s starting", touch->getName().c_str());
                if (!touch->start()) {
                    TT_LOG_E(TAG, "%s start failed", touch->getName().c_str());
                } else {
                    TT_LOG_I(TAG, "%s started", touch->getName().c_str());
                }
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
