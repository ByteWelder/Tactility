#include "SdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

constexpr auto CORES3_SDCARD_PIN_CS = GPIO_NUM_4;
constexpr auto CORES3_LCD_PIN_CS = GPIO_NUM_3;

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto configuration = std::make_unique<SpiSdCardDevice::Config>(
        CORES3_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getSyncLock(),
        std::vector {
            CORES3_LCD_PIN_CS
        },
        SPI3_HOST
    );

    return std::make_shared<SpiSdCardDevice>(
        std::move(configuration)
    );
}
