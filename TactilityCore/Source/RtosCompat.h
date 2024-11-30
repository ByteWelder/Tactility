#pragma once

/**
 * Compatibility includes for FreeRTOS.
 * Custom FreeRTOS from ESP-IDF prefixes paths with "freertos/",
 * but this isn't the normal behaviour for the regular FreeRTOS project.
 */

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#else
#include "FreeRTOS.h"
#endif
