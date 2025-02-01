#pragma once

#include <Tactility/Bundle.h>
#include <Tactility/Mutex.h>
#include <Tactility/service/wifi/Wifi.h>

namespace tt::app::wifiapsettings {

void start(const std::string& ssid);

} // namespace
