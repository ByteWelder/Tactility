#include "CoreS3SdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

#include <esp_vfs_fat.h>

#define CORES3_SDCARD_PIN_CS GPIO_NUM_4
#define CORES3_LCD_PIN_CS GPIO_NUM_3

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        CORES3_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getSyncLock(),
        {
            CORES3_LCD_PIN_CS
        },
        SPI3_HOST
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
