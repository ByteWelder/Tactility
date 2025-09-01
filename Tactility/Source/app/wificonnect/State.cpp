#include <Tactility/app/wificonnect/State.h>

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

void State::setApSettings(const service::wifi::settings::WifiApSettings& newSettings) {
    lock.lock();
    this->apSettings = newSettings;
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
