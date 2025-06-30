#include "TpagerSdCard.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

#include <esp_vfs_fat.h>

using tt::hal::sdcard::SpiSdCardDevice;

#define TDECK_SDCARD_PIN_CS GPIO_NUM_21
#define TDECK_LCD_PIN_CS GPIO_NUM_38
#define TDECK_RADIO_PIN_CS GPIO_NUM_36

std::shared_ptr<SdCardDevice> createTpagerSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        TDECK_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getSyncLock(),
        {TDECK_RADIO_PIN_CS,
         TDECK_LCD_PIN_CS}
    );

    auto* sdcard = (SdCardDevice*)new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
