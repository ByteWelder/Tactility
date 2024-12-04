#pragma once

namespace tt::app::wifimanage {

typedef void (*OnWifiToggled)(bool enable);
typedef void (*OnConnectSsid)(const char* ssid);
typedef void (*OnDisconnect)();
typedef void (*OnShowApSettings)(const char* ssid);

struct Bindings{
    OnWifiToggled onWifiToggled;
    OnConnectSsid onConnectSsid;
    OnDisconnect onDisconnect;
    OnShowApSettings onShowApSettings;
};

} // namespace
