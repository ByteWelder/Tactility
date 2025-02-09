#include "Core2SdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCard.h>

#include <esp_vfs_fat.h>

#define CORE2_SDCARD_PIN_CS GPIO_NUM_4
#define CORE2_LCD_PIN_CS GPIO_NUM_5

using tt::hal::sdcard::SpiSdCard;

std::shared_ptr<SdCard> createSdCard() {
    auto* configuration = new SpiSdCard::Config(
        CORE2_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCard::MountBehaviour::AtBoot,
        tt::lvgl::getLvglSyncLockable(),
        {
            CORE2_LCD_PIN_CS
        }
    );

    auto* sdcard = (SdCard*) new SpiSdCard(
        std::unique_ptr<SpiSdCard::Config>(configuration)
    );

    return std::shared_ptr<SdCard>(sdcard);
}
