#include "YellowSdCard.h"

#define TAG "twodotfour_sdcard"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/SpiSdCard.h>

#define SDCARD_SPI_HOST SPI3_HOST
#define SDCARD_PIN_CS GPIO_NUM_5

std::shared_ptr<SdCard> createYellowSdCard() {
    auto* configuration = new tt::hal::SpiSdCard::Config(
        SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCard::MountBehaviour::AtBoot,
        nullptr,
        std::vector<gpio_num_t>(),
        SDCARD_SPI_HOST
    );

    auto* sdcard = (SdCard*) new SpiSdCard(
        std::unique_ptr<SpiSdCard::Config>(configuration)
    );

    return std::shared_ptr<SdCard>(sdcard);
}

