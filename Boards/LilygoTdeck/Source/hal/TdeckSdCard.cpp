#include "TdeckSdCard.h"

#include "lvgl/LvglSync.h"
#include "hal/SpiSdCard.h"

#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define TDECK_SDCARD_SPI_FREQUENCY 800000U
#define TDECK_SDCARD_PIN_CS GPIO_NUM_39
#define TDECK_LCD_PIN_CS GPIO_NUM_12
#define TDECK_RADIO_PIN_CS GPIO_NUM_9

std::shared_ptr<SdCard> createTdeckSdCard() {
    auto* configuration = new tt::hal::SpiSdCard::Config(
        TDECK_SDCARD_SPI_FREQUENCY,
        TDECK_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCard::MountBehaviour::AtBoot,
        tt::lvgl::getLvglSyncLockable(),
        {
            TDECK_RADIO_PIN_CS,
            TDECK_LCD_PIN_CS
        }
    );

    auto* sdcard = (SdCard*) new SpiSdCard(
        std::unique_ptr<SpiSdCard::Config>(configuration)
    );

    return std::shared_ptr<SdCard>(sdcard);
}
