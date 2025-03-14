#include "WaveshareSdCard.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        GPIO_NUM_10,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
