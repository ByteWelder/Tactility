#include "YellowSdCard.h"
#include "YellowDisplayConstants.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createYellowSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        CYD_SDCARD_PIN_CS,
        GPIO_NUM_NC, // No card detect pin specified
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(),
        std::vector<gpio_num_t>(),
        CYD_SDCARD_SPI_HOST
    );

    return std::shared_ptr<SdCardDevice>(
        new SpiSdCardDevice(std::unique_ptr<SpiSdCardDevice::Config>(configuration))
    );
}
