#pragma once

#include <Tactility/hal/sdcard/SdCardDevice.h>

// Create SD card device for jc1060p470ciwy using SDMMC slot 0 (4-bit)
std::shared_ptr<tt::hal::sdcard::SdCardDevice> createSdCard();
