#include "UnPhoneSdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

#include <esp_vfs_fat.h>

#define UNPHONE_SDCARD_PIN_CS GPIO_NUM_43
#define UNPHONE_LCD_PIN_CS GPIO_NUM_48
#define UNPHONE_LORA_PIN_CS GPIO_NUM_44
#define UNPHONE_TOUCH_PIN_CS GPIO_NUM_38

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createUnPhoneSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        UNPHONE_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getLvglSyncLockable(),
        {
            UNPHONE_LORA_PIN_CS,
            UNPHONE_LCD_PIN_CS,
            UNPHONE_TOUCH_PIN_CS
        }
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
