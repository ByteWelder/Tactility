#include "SdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SdmmcDevice.h>

using tt::hal::sdcard::SdmmcDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto configuration = std::make_unique<SdmmcDevice::Config>(
        GPIO_NUM_36, //CLK
        GPIO_NUM_35, //CMD
        GPIO_NUM_37, //D0
        GPIO_NUM_33, //D1
        GPIO_NUM_38, //D2
        GPIO_NUM_34, //D3
        SdCardDevice::MountBehaviour::AtBoot
    );

    return std::make_shared<SdmmcDevice>(
        std::move(configuration)
    );
}
