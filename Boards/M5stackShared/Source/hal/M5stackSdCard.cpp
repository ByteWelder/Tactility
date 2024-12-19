#include "M5stackSdCard.h"

#include "lvgl/LvglSync.h"
#include "hal/SpiSdCard.h"

#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define SDCARD_PIN_CS GPIO_NUM_4
#define SDCARD_SPI_FREQUENCY 800000U

std::shared_ptr<SdCard> createTdeckSdcard() {
    auto* configuration = new tt::hal::SpiSdCard::Config(
        SDCARD_SPI_FREQUENCY,
        SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCard::MountBehaviourAtBoot,
        tt::lvgl::getLvglSyncLockable(),
        {
            GPIO_NUM_5
        }
    );

    auto* sdcard = (SdCard*) new SpiSdCard(
        std::unique_ptr<SpiSdCard::Config>(configuration)
    );

    return std::shared_ptr<SdCard>(sdcard);
}
