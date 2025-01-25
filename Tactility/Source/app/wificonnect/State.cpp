#include "app/wificonnect/State.h"
#include <cstring>

namespace tt::app::wificonnect {

void State::setConnectionError(bool error) {
    lock.lock();
    connectionError = error;
    lock.unlock();
}

bool State::hasConnectionError() const {
    lock.lock();
    auto result = connectionError;
    lock.unlock();
    return result;
}

void State::setApSettings(const service::wifi::settings::WifiApSettings* newSettings) {
    lock.lock();
    memcpy(&this->apSettings, newSettings, sizeof(service::wifi::settings::WifiApSettings));
    lock.unlock();
}

const service::wifi::settings::WifiApSettings& State::lockApSettings() {
    lock.lock();
    return apSettings;
}

void State::unlockApSettings() {
    lock.unlock();
}

void State::setConnecting(bool isConnecting) {
    lock.lock();
    connecting = isConnecting;
    lock.unlock();
}

bool State::isConnecting() const {
    lock.lock();
    auto result = connecting;
    lock.unlock();
    return result;
}

} // namespace
