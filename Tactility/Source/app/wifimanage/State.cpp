#include <Check.h>
#include "WifiManage.h"

namespace tt::app::wifimanage {

void State::setScanning(bool isScanning) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    scanning = isScanning;
    tt_check(mutex.release() == TtStatusOk);
}

void State::setRadioState(service::wifi::WifiRadioState state) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    radioState = state;
    tt_check(mutex.release() == TtStatusOk);
}

service::wifi::WifiRadioState State::getRadioState() const {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    auto result = radioState;
    tt_check(mutex.release() == TtStatusOk);
    return result;
}

bool State::isScanning() const {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    bool result = scanning;
    tt_check(mutex.release() == TtStatusOk);
    return result;
}

const std::vector<service::wifi::WifiApRecord>& State::lockApRecords() const {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    return apRecords;
}

void State::unlockApRecords() const {
    tt_check(mutex.release() == TtStatusOk);
}

void State::updateApRecords() {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    apRecords = service::wifi::getScanResults();
    tt_check(mutex.release() == TtStatusOk);
}

void State::setConnectSsid(std::string ssid) {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    connectSsid = ssid;
    tt_check(mutex.release() == TtStatusOk);
}

std::string State::getConnectSsid() const {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    auto result = connectSsid;
    tt_check(mutex.release() == TtStatusOk);
    return result;
}

} // namespace
