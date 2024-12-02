#pragma once

namespace tt::app::wifimanage {

typedef void (*OnWifiToggled)(bool enable);
typedef void (*OnConnectSsid)(const char* ssid);
typedef void (*OnDisconnect)();

struct Bindings{
    OnWifiToggled onWifiToggled;
    OnConnectSsid onConnectSsid;
    OnDisconnect onDisconnect;
};

} // namespace
