#pragma once

#include <Tactility/Mutex.h>
#include <Tactility/service/wifi/Wifi.h>
#include <Tactility/service/wifi/WifiSettings.h>

namespace tt::app::wificonnect {

class State {
    Mutex lock;
    service::wifi::settings::WifiApSettings apSettings = {
        .ssid = { 0 },
        .password = { 0 },
        .auto_connect = false
    };
    bool connectionError = false;
    bool connecting = false;
public:

    void setConnectionError(bool error);
    bool hasConnectionError() const;

    void setApSettings(const service::wifi::settings::WifiApSettings* newSettings);

    void setConnecting(bool isConnecting);
    bool isConnecting() const;
};

} // namespace
