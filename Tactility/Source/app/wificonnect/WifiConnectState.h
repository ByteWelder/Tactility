#pragma once

#include "service/wifi/Wifi.h"
#include "service/wifi/WifiSettings.h"

namespace tt::app::wificonnect {

/**
 * View's state
 */
typedef struct {
    service::wifi::settings::WifiApSettings settings;
    bool connection_error;
    bool is_connecting;
} WifiConnectState;

} // namespace
