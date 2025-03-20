#include "YellowSDCard.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>

static constexpr gpio_num_t SDCARD_PIN_CS = GPIO_NUM_5; // From schematic
static constexpr spi_host_device_t SDCARD_SPI_HOST = SPI3_HOST;

std::shared_ptr<tt::hal::sdcard::SdCardDevice> createYellowSDCard() {
    auto* configuration = new tt::hal::sdcard::SpiSdCardDevice::Config(
        SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        tt::hal::sdcard::SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(),
        std::vector<gpio_num_t>(),
        SDCARD_SPI_HOST
    );

    auto* sdcard = new tt::hal::sdcard::SpiSdCardDevice(
        std::unique_ptr<tt::hal::sdcard::SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<tt::hal::sdcard::SdCardDevice>(sdcard);
}
