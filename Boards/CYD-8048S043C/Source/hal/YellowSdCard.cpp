#include "YellowSdCard.h"

#define TAG "cyd8048s043c_sdcard"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

#define SDCARD_SPI_HOST SPI2_HOST
#define SDCARD_PIN_CS GPIO_NUM_10

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createYellowSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(),
        std::vector<gpio_num_t>(),
        SDCARD_SPI_HOST
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}

