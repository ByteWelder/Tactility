#include "UnPhoneSdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/SpiSdCard.h>

#include <esp_vfs_fat.h>

#define UNPHONE_SDCARD_PIN_CS GPIO_NUM_43
#define UNPHONE_LCD_PIN_CS GPIO_NUM_48
#define UNPHONE_LORA_PIN_CS GPIO_NUM_44
#define UNPHONE_TOUCH_PIN_CS GPIO_NUM_38

std::shared_ptr<SdCard> createUnPhoneSdCard() {
    auto* configuration = new tt::hal::SpiSdCard::Config(
        UNPHONE_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCard::MountBehaviour::AtBoot,
        tt::lvgl::getLvglSyncLockable(),
        {
            UNPHONE_LORA_PIN_CS,
            UNPHONE_LCD_PIN_CS,
            UNPHONE_TOUCH_PIN_CS
        }
    );

    auto* sdcard = (SdCard*) new SpiSdCard(
        std::unique_ptr<SpiSdCard::Config>(configuration)
    );

    return std::shared_ptr<SdCard>(sdcard);
}
