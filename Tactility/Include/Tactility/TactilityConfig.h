#pragma once

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#define TT_CONFIG_FORCE_ONSCREEN_KEYBOARD false // for development/debug purposes

#ifdef ESP_PLATFORM
#define TT_FEATURE_SCREENSHOT_ENABLED (CONFIG_LV_USE_SNAPSHOT == 1 && CONFIG_SPIRAM_USE_MALLOC == 1)
#else // Sim
#define TT_FEATURE_SCREENSHOT_ENABLED true
#endif

namespace tt::config {

constexpr auto SHOW_SYSTEM_PARTITION = false;

}
