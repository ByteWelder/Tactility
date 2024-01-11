#pragma once

#include <stdbool.h>

typedef void (*OnWifiToggled)(bool enable);
typedef void (*OnShowConnectDialog)(const char* ssid, void* context);
typedef void (*OnHideConnectDialog)(void* context);
typedef void (*OnConnectSsid)(const char* ssid, const char* password, void* context);

typedef struct {
    OnWifiToggled on_wifi_toggled;
    OnShowConnectDialog on_show_connect_dialog;
    void* on_show_connect_dialog_context;
    OnHideConnectDialog on_hide_connect_dialog;
    void* on_hide_connect_dialog_context;
    OnConnectSsid on_connect_ssid;
    void* on_connect_ssid_context;
} WifiBindings;
