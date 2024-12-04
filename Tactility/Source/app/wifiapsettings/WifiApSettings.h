#pragma once

#include "Bundle.h"
#include "Mutex.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wifiapsettings {

void start(const std::string& ssid);

} // namespace
