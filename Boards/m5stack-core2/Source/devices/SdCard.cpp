#include "SdCard.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

constexpr auto CORE2_SDCARD_PIN_CS = GPIO_NUM_4;
constexpr auto CORE2_LCD_PIN_CS = GPIO_NUM_5;

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto configuration = std::make_unique<SpiSdCardDevice::Config>(
        CORE2_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getSyncLock(),
        std::vector {
            CORE2_LCD_PIN_CS
        }
    );

    return std::make_shared<SpiSdCardDevice>(
        std::move(configuration)
    );
}
