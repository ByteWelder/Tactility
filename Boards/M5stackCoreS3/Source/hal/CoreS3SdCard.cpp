#include "CoreS3SdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCard.h>

#include <esp_vfs_fat.h>

#define CORES3_SDCARD_PIN_CS GPIO_NUM_4
#define CORES3_LCD_PIN_CS GPIO_NUM_3

using tt::hal::sdcard::SpiSdCard;

std::shared_ptr<SdCard> createSdCard() {
    auto* configuration = new SpiSdCard::Config(
        CORES3_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCard::MountBehaviour::AtBoot,
        tt::lvgl::getLvglSyncLockable(),
        {
            CORES3_LCD_PIN_CS
        },
        SPI3_HOST
    );

    auto* sdcard = (SdCard*) new SpiSdCard(
        std::unique_ptr<SpiSdCard::Config>(configuration)
    );

    return std::shared_ptr<SdCard>(sdcard);
}
