#pragma once

#include "service/wifi/Wifi.h"
#include "Mutex.h"

namespace tt::app::wifimanage {

/**
 * View's state
 */
class State {

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    bool scanning = false;
    bool scannedAfterRadioOn = false;
    service::wifi::RadioState radioState;
    std::vector<service::wifi::ApRecord> apRecords;
    std::string connectSsid;

public:
    State() = default;

    void setScanning(bool isScanning);
    bool isScanning() const;

    bool hasScannedAfterRadioOn() const { return scannedAfterRadioOn; }

    void setRadioState(service::wifi::RadioState state);
    service::wifi::RadioState getRadioState() const;

    void updateApRecords();

    void withApRecords(const std::function<void(const std::vector<service::wifi::ApRecord>&)>& onApRecords) const {
        mutex.withLock([&]() {
            onApRecords(apRecords);
        });
    }

    void setConnectSsid(const std::string& ssid);
    std::string getConnectSsid() const;
};

} // namespace
