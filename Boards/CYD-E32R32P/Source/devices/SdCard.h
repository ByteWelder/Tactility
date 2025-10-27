#pragma once

#include <Tactility/hal/sdcard/SdCardDevice.h>
#include "driver/gpio.h"
#include "driver/spi_common.h"

using tt::hal::sdcard::SdCardDevice;

// SD card (microSD)
constexpr auto SD_CS_PIN = GPIO_NUM_5;
constexpr auto SD_SPI_HOST = SPI3_HOST;

std::shared_ptr<SdCardDevice> createSdCard();
