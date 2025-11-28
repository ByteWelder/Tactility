#pragma once

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#ifdef CONFIG_ESP_WIFI_ENABLED

#include "Tactility/service/espnow/EspNow.h"

namespace tt::service::espnow {

bool initWifi(const EspNowConfig& config);

bool deinitWifi();

}

#endif