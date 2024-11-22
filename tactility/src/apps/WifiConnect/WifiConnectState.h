#pragma once

#include "App.h"
#include "services/Wifi/Wifi.h"
#include "services/Wifi/WifiSettings.h"

namespace tt::app::wifi_connect {

/**
 * View's state
 */
typedef struct {
    service::wifi::settings::WifiApSettings settings;
    bool connection_error;
    bool is_connecting;
} WifiConnectState;

} // namespace
