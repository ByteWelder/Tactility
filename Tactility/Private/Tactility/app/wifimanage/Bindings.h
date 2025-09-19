#pragma once

#include <string>

namespace tt::app::wifimanage {

typedef void (*OnWifiToggled)(bool enable);
typedef void (*OnConnectSsid)(const std::string& ssid);
typedef void (*OnDisconnect)();
typedef void (*OnShowApSettings)(const std::string& ssid);
typedef void (*OnConnectToHidden)();

struct Bindings{
    OnWifiToggled onWifiToggled;
    OnConnectSsid onConnectSsid;
    OnDisconnect onDisconnect;
    OnShowApSettings onShowApSettings;
    OnConnectToHidden onConnectToHidden;
};

} // namespace
