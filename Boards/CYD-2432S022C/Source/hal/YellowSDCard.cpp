#include "YellowSDCard.h"
#include "CYD2432S022CConstants.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <esp_log.h>


using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createYellowSDCard() {
    ESP_LOGI("SDCard", "Heap free before SD card init: %d bytes",
             heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    auto* configuration = new SpiSdCardDevice::Config(
        CYD_2432S022C_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        nullptr,
        std::vector<gpio_num_t>(),
        CYD_2432S022C_SDCARD_SPI_HOST
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    ESP_LOGI("SDCard", "Heap free after SD card init: %d bytes",
             heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
    return std::shared_ptr<SdCardDevice>(sdcard);
}
