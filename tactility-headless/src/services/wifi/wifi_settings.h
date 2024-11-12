#pragma once

#include <stdbool.h>
#include "wifi_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ssid[TT_WIFI_SSID_LIMIT + 1]; // Add extra character for null termination
    char secret[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT + 1]; // Add extra character for null termination
    bool auto_connect;
} WifiApSettings;

void tt_wifi_settings_init();

bool tt_wifi_settings_contains(const char* ssid);

bool tt_wifi_settings_load(const char* ssid, WifiApSettings* settings);

bool tt_wifi_settings_save(const WifiApSettings* settings);

bool tt_wifi_settings_remove(const char* ssid);

#ifdef __cplusplus
}
#endif
