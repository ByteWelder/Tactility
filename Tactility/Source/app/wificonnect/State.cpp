#include "app/wificonnect/State.h"
#include "Check.h"
#include <cstring>

namespace tt::app::wificonnect {

void State::setConnectionError(bool error) {
    tt_check(lock.acquire(TtWaitForever) == TtStatusOk);
    connectionError = error;
    tt_check(lock.release() == TtStatusOk);
}

bool State::hasConnectionError() const {
    tt_check(lock.acquire(TtWaitForever) == TtStatusOk);
    auto result = connectionError;
    tt_check(lock.release() == TtStatusOk);
    return result;
}

void State::setApSettings(const service::wifi::settings::WifiApSettings* newSettings) {
    tt_check(lock.acquire(TtWaitForever) == TtStatusOk);
    memcpy(&this->apSettings, newSettings, sizeof(service::wifi::settings::WifiApSettings));
    tt_check(lock.release() == TtStatusOk);
}

const service::wifi::settings::WifiApSettings& State::lockApSettings() {
    tt_check(lock.acquire(TtWaitForever) == TtStatusOk);
    return apSettings;
}

void State::unlockApSettings() {
    tt_check(lock.release() == TtStatusOk);
}

void State::setConnecting(bool isConnecting) {
    tt_check(lock.acquire(TtWaitForever) == TtStatusOk);
    connecting = isConnecting;
    tt_check(lock.release() == TtStatusOk);
}

bool State::isConnecting() const {
    tt_check(lock.acquire(TtWaitForever) == TtStatusOk);
    auto result = connecting;
    tt_check(lock.release() == TtStatusOk);
    return result;
}

} // namespace
