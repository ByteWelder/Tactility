#include "Tactility/hal/Configuration.h"
#include "Tactility/hal/Device.h"
#include "Tactility/hal/gps/GpsInit.h"
#include "Tactility/hal/i2c/I2cInit.h"
#include "Tactility/hal/power/PowerDevice.h"
#include "Tactility/hal/spi/SpiInit.h"
#include "Tactility/hal/uart/UartInit.h"

#include <Tactility/kernel/SystemEvents.h>

#define TAG "hal"

#define TT_SDCARD_MOUNT_POINT "/sdcard"

namespace tt::hal {

void initDevices(const Configuration& configuration) {
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
    }

    // TODO: Move
    auto sdcards = hal::findDevices<sdcard::SdCardDevice>(Device::Type::SdCard);
    if (!sdcards.empty()) {
        if (sdcards.size() == 1) {
            // Fixed mount path name
            auto sdcard = sdcards[0];
            TT_LOG_I(TAG, "Mounting sdcard at %s", TT_SDCARD_MOUNT_POINT);
            if (!sdcard->mount(TT_SDCARD_MOUNT_POINT)) {
                TT_LOG_W(TAG, "SD card mount failed (init can continue)");
            }
        } else {
            // Numbered mount path name
            for (int i = 0; i < sdcards.size(); i++) {
                auto sdcard = sdcards[i];
                std::string mount_path = TT_SDCARD_MOUNT_POINT + std::to_string(i);
                TT_LOG_I(TAG, "Mounting sdcard at %d", mount_path.c_str());
                if (!sdcard->mount(mount_path)) {
                    TT_LOG_W(TAG, "SD card mount failed (init can continue)");
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

    initDevices(configuration);

    kernel::publishSystemEvent(kernel::SystemEvent::BootInitHalEnd);
}

} // namespace
