#include <Tactility/app/wifimanage/WifiManagePrivate.h>

namespace tt::app::wifimanage {

void State::setScanning(bool isScanning) {
    mutex.lock();
    scanning = isScanning;
    scannedAfterRadioOn |= isScanning;
    mutex.unlock();
}

void State::setRadioState(service::wifi::RadioState state) {
    mutex.lock();
    radioState = state;
    if (radioState == service::wifi::RadioState::Off) {
        scannedAfterRadioOn = false;
    }
    mutex.unlock();
}

service::wifi::RadioState State::getRadioState() const {
    mutex.lock();
    auto result = radioState;
    mutex.unlock();
    return result;
}

bool State::isScanning() const {
    mutex.lock();
    bool result = scanning;
    mutex.unlock();
    return result;
}

void State::updateApRecords() {
    mutex.lock();
    apRecords = service::wifi::getScanResults();
    mutex.unlock();
}

void State::setConnectSsid(const std::string& ssid) {
    mutex.lock();
    connectSsid = ssid;
    mutex.unlock();
}

std::string State::getConnectSsid() const {
    mutex.lock();
    auto result = connectSsid;
    return result;
}

} // namespace
