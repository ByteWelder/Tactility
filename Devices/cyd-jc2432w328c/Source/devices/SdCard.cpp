#include "SdCard.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

constexpr auto SDCARD_SPI_HOST = SPI3_HOST;
constexpr auto SDCARD_PIN_CS = GPIO_NUM_5;

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto configuration = std::make_unique<SpiSdCardDevice::Config>(
        SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(tt::Mutex::Type::Recursive),
        std::vector<gpio_num_t>(),
        SDCARD_SPI_HOST
    );

    return std::make_shared<SpiSdCardDevice>(
        std::move(configuration)
    );
}

