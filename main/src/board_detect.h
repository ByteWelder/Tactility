#pragma once

// Supported hardware:
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#include "lilygo_tdeck.h"
#elif defined(CONFIG_IDF_TARGET_ESP32)
#include "yellow_board.h"
#endif

#if defined(CONFIG_IDF_TARGET_ESP32S3)
    #define HARDWARE_AUTO_DETECT &lilygo_tdeck
#elif defined(CONFIG_IDF_TARGET_ESP32)
    #define HARDWARE_AUTO_DETECT &yellow_board_24inch_cap
#else
    #error "HARDWARE_AUTO_DETECT not supported for ESP target: manually set HardwareConfig for your target or update the target via 'idf.py menuconfig'"
#endif
