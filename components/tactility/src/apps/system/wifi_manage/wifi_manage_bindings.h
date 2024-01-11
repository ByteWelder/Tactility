#pragma once

#include <stdbool.h>

typedef void (*OnWifiToggled)(bool enable);
typedef void (*OnConnectSsid)(const char* ssid, void* context);

typedef struct {
    OnWifiToggled on_wifi_toggled;
    OnConnectSsid on_connect_ssid;
    void* on_connect_ssid_context;
} WifiManageBindings;
