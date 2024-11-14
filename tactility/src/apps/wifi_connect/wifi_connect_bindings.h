#pragma once

#include "services/wifi/wifi_settings.h"
#include <stdbool.h>

typedef void (*OnConnectSsid)(const WifiApSettings* settings, bool store, void* context);

typedef struct {
    OnConnectSsid on_connect_ssid;
    void* on_connect_ssid_context;
} WifiConnectBindings;
