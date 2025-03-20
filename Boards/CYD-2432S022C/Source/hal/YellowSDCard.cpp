#include "YellowSDCard.h"
#include "CYD2432S022CConstants.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

std::shared_ptr<tt::hal::sdcard::SdCardDevice> createYellowSDCard() {
    auto* configuration = new tt::hal::sdcard::SpiSdCardDevice::Config(
        CYD_2432S022C_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        tt::hal::sdcard::SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(),
        std::vector<gpio_num_t>(),
        CYD_2432S022C_SDCARD_SPI_HOST
    );

    auto* sdcard = new tt::hal::sdcard::SpiSdCardDevice(
        std::unique_ptr<tt::hal::sdcard::SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<tt::hal::sdcard::SdCardDevice>(sdcard);
}
