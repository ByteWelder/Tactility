#pragma once

#include <Tactility/service/wifi/Wifi.h>
#include <Tactility/Mutex.h>

namespace tt::app::wifimanage {

/**
 * View's state
 */
class State final {

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

    template <std::invocable<const std::vector<service::wifi::ApRecord>&> Func>
    void withApRecords(Func&& onApRecords) const {
        mutex.withLock([&] {
            std::invoke(std::forward<Func>(onApRecords), apRecords);
        });
    }

    void setConnectSsid(const std::string& ssid);
    std::string getConnectSsid() const;
};

} // namespace
