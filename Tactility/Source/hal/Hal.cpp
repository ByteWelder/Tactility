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

void init(const Configuration& configuration) {
    kernel::systemEventPublish(kernel::SystemEvent::BootInitHalBegin);

    kernel::systemEventPublish(kernel::SystemEvent::BootInitI2cBegin);
    tt_check(i2c::init(configuration.i2c), "I2C init failed");
    kernel::systemEventPublish(kernel::SystemEvent::BootInitI2cEnd);

    kernel::systemEventPublish(kernel::SystemEvent::BootInitSpiBegin);
    tt_check(spi::init(configuration.spi), "SPI init failed");
    kernel::systemEventPublish(kernel::SystemEvent::BootInitSpiEnd);

    kernel::systemEventPublish(kernel::SystemEvent::BootInitUartBegin);
    tt_check(uart::init(configuration.uart), "UART init failed");
    kernel::systemEventPublish(kernel::SystemEvent::BootInitUartEnd);

    if (configuration.initBoot != nullptr) {
        TT_LOG_I(TAG, "Init power");
        tt_check(configuration.initBoot(), "Init power failed");
    }

    if (configuration.sdcard != nullptr) {
        TT_LOG_I(TAG, "Mounting sdcard");
        if (!configuration.sdcard->mount(TT_SDCARD_MOUNT_POINT)) {
            TT_LOG_W(TAG, "SD card mount failed (init can continue)");
        }
        hal::registerDevice(configuration.sdcard);
    }

    if (configuration.power != nullptr) {
        std::shared_ptr<tt::hal::power::PowerDevice> power = configuration.power();
        hal::registerDevice(power);
    }

    kernel::systemEventPublish(kernel::SystemEvent::BootInitHalEnd);
}

} // namespace
