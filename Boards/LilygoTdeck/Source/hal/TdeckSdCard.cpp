#include "TdeckSdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCard.h>

#include <esp_vfs_fat.h>

using tt::hal::sdcard::SpiSdCard;

#define TDECK_SDCARD_PIN_CS GPIO_NUM_39
#define TDECK_LCD_PIN_CS GPIO_NUM_12
#define TDECK_RADIO_PIN_CS GPIO_NUM_9

std::shared_ptr<SdCard> createTdeckSdCard() {
    auto* configuration = new SpiSdCard::Config(
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
