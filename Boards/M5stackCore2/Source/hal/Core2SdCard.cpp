#include "Core2SdCard.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

#include <esp_vfs_fat.h>

#define CORE2_SDCARD_PIN_CS GPIO_NUM_4
#define CORE2_LCD_PIN_CS GPIO_NUM_5

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        CORE2_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getLvglSyncLock(),
        {
            CORE2_LCD_PIN_CS
        }
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
