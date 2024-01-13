#pragma once

#include <stdbool.h>

typedef void (*OnConnectSsid)(const char* ssid, const char* password, void* context);

typedef struct {
    OnConnectSsid on_connect_ssid;
    void* on_connect_ssid_context;
} WifiConnectBindings;
