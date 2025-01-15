#include "Core2SdCard.h"

#include "lvgl/LvglSync.h"
#include "hal/SpiSdCard.h"

#include <esp_vfs_fat.h>

#define CORE2_SDCARD_SPI_FREQUENCY 800000U
#define CORE2_SDCARD_PIN_CS GPIO_NUM_4
#define CORE2_LCD_PIN_CS GPIO_NUM_5

std::shared_ptr<SdCard> createSdCard() {
    auto* configuration = new tt::hal::SpiSdCard::Config(
        CORE2_SDCARD_SPI_FREQUENCY,
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
