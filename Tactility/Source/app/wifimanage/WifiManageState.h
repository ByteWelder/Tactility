#pragma once

#include "service/wifi/Wifi.h"

namespace tt::app::wifimanage {

/**
 * View's state
 */
typedef struct {
    bool scanning;
    service::wifi::WifiRadioState radio_state;
    std::string connect_ssid;
    std::vector<service::wifi::WifiApRecord> ap_records;
} WifiManageState;

} // namespace
