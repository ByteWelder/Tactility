#include "Sdcard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

using tt::hal::sdcard::SpiSdCardDevice;

constexpr auto SDCARD_PIN_CS = GPIO_NUM_14;
constexpr auto LCD_PIN_CS = GPIO_NUM_21;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto configuration = std::make_unique<SpiSdCardDevice::Config>(
        SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::hal::spi::getLock(SPI3_HOST),
        std::vector { LCD_PIN_CS },
        SPI3_HOST
    );

    return std::make_shared<SpiSdCardDevice>(
        std::move(configuration)
    );
}
