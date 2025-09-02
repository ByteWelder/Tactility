#include "SdCard.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto config = std::make_unique<SpiSdCardDevice::Config>(
        GPIO_NUM_5,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(),
        std::vector<gpio_num_t>(),
        SPI3_HOST
    );

    return std::make_shared<SpiSdCardDevice>(std::move(config));
}

