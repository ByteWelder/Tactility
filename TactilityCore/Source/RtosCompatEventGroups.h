#pragma once

/**
 * See explanation in RtosCompat.h
 */

#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#else
#include "FreeRTOS.h"
#include "event_groups.h"
#endif

