#include "YellowSDCard.h"
#include "CYD2432S022CConstants.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createYellowSDCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        CYD_2432S022C_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        nullptr,
        std::vector<gpio_num_t>(),
        CYD_2432S022C_SDCARD_SPI_HOST
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
