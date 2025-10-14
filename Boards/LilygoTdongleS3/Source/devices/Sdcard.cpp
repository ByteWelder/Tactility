#include "Sdcard.h"

#include <Tactility/hal/sdcard/SdmmcDevice.h>
#include <Tactility/lvgl/LvglSync.h>

using tt::hal::sdcard::SdmmcDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto configuration = std::make_unique<SdmmcDevice::Config>(
        GPIO_NUM_12,
        GPIO_NUM_16,
        GPIO_NUM_14,
        GPIO_NUM_17,
        GPIO_NUM_21,
        GPIO_NUM_18,
        SdCardDevice::MountBehaviour::AtBoot
    );

    return std::make_shared<SdmmcDevice>(
        std::move(configuration)
    );
}
