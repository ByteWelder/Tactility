#pragma once

#include "service/wifi/Wifi.h"
#include "Mutex.h"

namespace tt::app::wifimanage {

/**
 * View's state
 */
class State {

    Mutex mutex = Mutex(MutexTypeRecursive);
    bool scanning;
    service::wifi::WifiRadioState radioState;
    std::vector<service::wifi::WifiApRecord> apRecords;
    std::string connectSsid;

public:
    State() {}

    void setScanning(bool isScanning);
    bool isScanning() const;

    void setRadioState(service::wifi::WifiRadioState state);
    service::wifi::WifiRadioState getRadioState() const;

    void updateApRecords();

    const std::vector<service::wifi::WifiApRecord>& lockApRecords() const;
    void unlockApRecords() const;

    void setConnectSsid(std::string ssid);
    std::string getConnectSsid() const;
};

} // namespace
