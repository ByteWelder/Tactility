#pragma once

namespace tt::app::wifimanage {

typedef void (*OnWifiToggled)(bool enable);
typedef void (*OnConnectSsid)(const char* ssid);
typedef void (*OnDisconnect)();

typedef struct {
    OnWifiToggled on_wifi_toggled;
    OnConnectSsid on_connect_ssid;
    OnDisconnect on_disconnect;
} WifiManageBindings;

} // namespace
