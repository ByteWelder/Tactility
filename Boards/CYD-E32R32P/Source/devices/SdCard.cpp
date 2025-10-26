#include "SdCard.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

using tt::hal::sdcard::SpiSdCardDevice;
using SdCardDevice = tt::hal::sdcard::SdCardDevice;

std::shared_ptr<SdCardDevice> createSdCard() {
    auto configuration = std::make_unique<SpiSdCardDevice::Config>(
        CYD_SD_CS_PIN,                         // CS pin (IO5 on the module)
        GPIO_NUM_NC,                           // MOSI override: leave NC to use SPI host pins
        GPIO_NUM_NC,                           // MISO override: leave NC
        GPIO_NUM_NC,                           // SCLK override: leave NC
        SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(tt::Mutex::Type::Recursive),
        std::vector<gpio_num_t>(),
        CYD_SD_SPI_HOST                        // SPI host for SD card (SPI3_HOST)
    );

    return std::make_shared<SpiSdCardDevice>(
        std::move(configuration)
    );
}