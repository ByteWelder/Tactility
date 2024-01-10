#pragma once

#include <stdbool.h>

typedef void (*OnWifiToggled)(bool);
typedef void (*OnConnectSsid)(const char* ssid, void*);

typedef struct {
    OnWifiToggled on_wifi_toggled;
    void* on_connect_ssid_context;
    OnConnectSsid on_connect_ssid;
} WifiBindings;
