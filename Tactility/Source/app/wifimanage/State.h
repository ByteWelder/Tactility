#pragma once

#include "service/wifi/Wifi.h"
#include "Mutex.h"

namespace tt::app::wifimanage {

/**
 * View's state
 */
class State {

    Mutex mutex;
    bool scanning;
    service::wifi::WifiRadioState radioState;
    std::vector<service::wifi::WifiApRecord> apRecords;
    std::string connectSsid;

public:
    State() {}


    void setScanning(bool isScanning);
    bool isScanning() const { return scanning; }

    void setRadioState(service::wifi::WifiRadioState state);
    service::wifi::WifiRadioState getRadioState() const { return radioState; }

    void updateApRecords();
    std::vector<service::wifi::WifiApRecord> getApRecords() const { return apRecords; }

    void setConnectSsid(std::string ssid) { connectSsid = ssid; }
    std::string getConnectSsid() const { return connectSsid; }
};

} // namespace
