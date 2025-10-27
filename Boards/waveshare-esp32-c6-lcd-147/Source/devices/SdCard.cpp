#include "SdCard.h"

#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/hal/spi/Spi.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    // SD card shares SPI bus with display (SPI2_HOST)
    // CS pin is GPIO4, need to protect display CS during SD operations
    auto configuration = std::make_unique<SpiSdCardDevice::Config>(
        GPIO_NUM_4,   // CS pin for SD card
        GPIO_NUM_NC,  // CD (card detect) pin - not used
        GPIO_NUM_NC,  // WP (write protect) pin - not used
        GPIO_NUM_NC,  // Power pin - not used
        SdCardDevice::MountBehaviour::AtBoot,
        tt::hal::spi::getLock(SPI2_HOST),  // Use same lock as display
        std::vector<gpio_num_t> { GPIO_NUM_14 },  // Assert display CS high during SD operations
        SPI2_HOST
    );

    return std::make_shared<SpiSdCardDevice>(
        std::move(configuration)
    );
}
