#pragma once

#include "service/wifi/WifiSettings.h"

namespace tt::app::wificonnect {

typedef void (*OnConnectSsid)(const service::wifi::settings::WifiApSettings* settings, bool store, void* context);

typedef struct {
    OnConnectSsid onConnectSsid;
    void* onConnectSsidContext;
} Bindings;

} // namespace
