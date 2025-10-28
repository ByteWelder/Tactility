#pragma once

#include "Tactility/hal/sdcard/SdCardDevice.h"
#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <memory>

using tt::hal::sdcard::SdCardDevice;

// SD card configuration (shares SPI bus with display)
constexpr auto SD_PIN_CS = GPIO_NUM_4;

std::shared_ptr<SdCardDevice> createSdCard();
