#include "YellowSDCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

#include <esp_vfs_fat.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    ESP_LOGI("sdcard", "Creating SD card on SPI3_HOST");
    auto* configuration = new SpiSdCardDevice::Config(
        GPIO_NUM_5,  // CS pin for CYD
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        tt::lvgl::getSyncLock(),
        {},
        SPI3_HOST
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    ESP_LOGI("sdcard", "SD card created");
    return std::shared_ptr<SdCardDevice>(sdcard);
}
