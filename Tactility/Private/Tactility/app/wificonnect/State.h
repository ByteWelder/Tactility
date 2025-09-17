#pragma once

#include <Tactility/Mutex.h>
#include <Tactility/service/wifi/WifiApSettings.h>

namespace tt::app::wificonnect {

class State final {
    Mutex lock;
    service::wifi::settings::WifiApSettings apSettings;
    bool connectionError = false;
    bool connecting = false;
public:

    void setConnectionError(bool error);
    bool hasConnectionError() const;

    void setApSettings(const service::wifi::settings::WifiApSettings& newSettings);

    void setConnecting(bool isConnecting);
    bool isConnecting() const;
};

} // namespace
