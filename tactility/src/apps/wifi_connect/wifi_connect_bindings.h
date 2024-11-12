#pragma once

#include <stdbool.h>

typedef void (*OnConnectSsid)(const char ssid[TT_WIFI_SSID_LIMIT], const char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT], void* context);

typedef struct {
    OnConnectSsid on_connect_ssid;
    void* on_connect_ssid_context;
} WifiConnectBindings;
