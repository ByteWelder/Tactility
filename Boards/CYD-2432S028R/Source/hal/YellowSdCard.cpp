#include "YellowSdCard.h"
#include "YellowConstants.h"
#include <Tactility/hal/sdcard/SpiSdCardDevice.h>
#include <Tactility/lvgl/LvglSync.h>

using tt::hal::sdcard::SpiSdCardDevice;

std::shared_ptr<SdCardDevice> createYellowSdCard() {
    auto* configuration = new SpiSdCardDevice::Config(
        CYD2432S028R_SDCARD_PIN_CS,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        GPIO_NUM_NC,
        SdCardDevice::MountBehaviour::AtBoot,
        std::make_shared<tt::Mutex>(),
        std::vector<gpio_num_t>(),
        CYD2432S028R_SDCARD_SPI_HOST
    );

    auto* sdcard = (SdCardDevice*) new SpiSdCardDevice(
        std::unique_ptr<SpiSdCardDevice::Config>(configuration)
    );

    return std::shared_ptr<SdCardDevice>(sdcard);
}
