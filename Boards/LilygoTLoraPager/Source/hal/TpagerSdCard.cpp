#include "TpagerSdCard.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

#include <esp_vfs_fat.h>

using tt::hal::sdcard::SpiSdCardDevice;

#define TPAGER_SDCARD_PIN_CS GPIO_NUM_21
#define TPAGER_LCD_PIN_CS GPIO_NUM_38
#define TPAGER_RADIO_PIN_CS GPIO_NUM_36

std::shared_ptr<SdCardDevice> createTpagerSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        TPAGER_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getSyncLock(),
        {TPAGER_RADIO_PIN_CS,
         TPAGER_LCD_PIN_CS}
    );

    auto* sdcard = (SdCardDevice*)new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
