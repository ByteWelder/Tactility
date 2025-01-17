#include "CoreS3SdCard.h"

#include "lvgl/LvglSync.h"
#include "hal/SpiSdCard.h"

#include <esp_vfs_fat.h>

#define CORES3_SDCARD_SPI_FREQUENCY 800000U
#define CORES3_SDCARD_PIN_CS GPIO_NUM_4
#define CORES3_LCD_PIN_CS GPIO_NUM_3

std::shared_ptr<SdCard> createSdCard() {
    auto* configuration = new tt::hal::SpiSdCard::Config(
        CORES3_SDCARD_SPI_FREQUENCY,
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
