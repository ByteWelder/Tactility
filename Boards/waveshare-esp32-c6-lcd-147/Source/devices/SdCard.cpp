#include "SdCard.h"
#include "Display.h"

#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/hal/spi/Spi.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    // SD card shares SPI bus with display (SPI2_HOST)
    // CS pin is GPIO4, need to protect display CS during SD operations
    auto configuration = std::make_unique<SpiSdCardDevice::Config>(
        SD_PIN_CS,    // CS pin for SD card
        GPIO_NUM_NC,  // CD (card detect) pin - not used
        GPIO_NUM_NC,  // WP (write protect) pin - not used
        GPIO_NUM_NC,  // Power pin - not used
        SdCardDevice::MountBehaviour::AtBoot,
        tt::hal::spi::getLock(LCD_SPI_HOST),  // Use same lock as display
        std::vector<gpio_num_t> { LCD_PIN_CS },  // Assert display CS high during SD operations
        LCD_SPI_HOST
    );

    return std::make_shared<SpiSdCardDevice>(
        std::move(configuration)
    );
}
