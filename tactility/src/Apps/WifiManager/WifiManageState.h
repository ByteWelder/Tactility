#pragma once

#include "Services/Wifi/Wifi.h"

namespace tt::app::wifi_manage {

#define WIFI_SCAN_AP_RECORD_COUNT 16

/**
 * View's state
 */
typedef struct {
    bool scanning;
    service::wifi::WifiRadioState radio_state;
    uint8_t connect_ssid[33];
    service::wifi::WifiApRecord ap_records[WIFI_SCAN_AP_RECORD_COUNT];
    uint16_t ap_records_count;
} WifiManageState;

} // namespace
