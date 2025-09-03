#include "SdCard.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto config = std::make_unique<SpiSdCardDevice::Config>(
        GPIO_NUM_10,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot
    );

    auto sdcard = std::make_shared<SpiSdCardDevice>(
        std::move(config)
    );

    return std::static_pointer_cast<SdCardDevice>(sdcard);
}
