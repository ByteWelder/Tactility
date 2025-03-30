#pragma once

#include "Tactility/service/espnow/EspNow.h"

namespace tt::service::espnow {

bool initWifi(const EspNowConfig& config);

bool deinitWifi();

}
