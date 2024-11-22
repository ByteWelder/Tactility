#pragma once

#include "app.h"
#include "services/wifi/wifi.h"
#include "services/wifi/wifi_settings.h"

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
