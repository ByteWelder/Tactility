#include "TdeckSdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

#include <esp_vfs_fat.h>

using tt::hal::sdcard::SpiSdCardDevice;

#define TDECK_SDCARD_PIN_CS GPIO_NUM_39
#define TDECK_LCD_PIN_CS GPIO_NUM_12
#define TDECK_RADIO_PIN_CS GPIO_NUM_9

std::shared_ptr<SdCardDevice> createTdeckSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        TDECK_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getSyncLock(),
        {
            TDECK_RADIO_PIN_CS,
            TDECK_LCD_PIN_CS
        }
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
