#pragma once

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

#define TT_CONFIG_FORCE_ONSCREEN_KEYBOARD false // for development/debug purposes
#define TT_SCREENSHOT_MODE false // for taking screenshots (e.g. forces SD card presence and Files tree on simulator)

#ifdef ESP_PLATFORM
#define TT_FEATURE_SCREENSHOT_ENABLED (CONFIG_LV_USE_SNAPSHOT == 1 && CONFIG_SPIRAM_USE_MALLOC == 1)
#else // Sim
#define TT_FEATURE_SCREENSHOT_ENABLED true
#endif
