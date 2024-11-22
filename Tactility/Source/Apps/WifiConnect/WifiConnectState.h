#pragma once

#include "App.h"
#include "Services/Wifi/Wifi.h"
#include "Services/Wifi/WifiSettings.h"

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
