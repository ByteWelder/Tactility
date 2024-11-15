#pragma once

#include "wifi_globals.h"

/**
 * This struct is stored as-is into NVS flash.
 *
 * The SSID and secret are increased by 1 byte to facilitate string null termination.
 * This makes it easier to use the char array as a string in various places.
 */
typedef struct {
    char ssid[TT_WIFI_SSID_LIMIT + 1];
    char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT + 1];
    bool auto_connect;
} WifiApSettings;

void tt_wifi_settings_init();

bool tt_wifi_settings_contains(const char* ssid);

bool tt_wifi_settings_load(const char* ssid, WifiApSettings* settings);

bool tt_wifi_settings_save(const WifiApSettings* settings);

bool tt_wifi_settings_remove(const char* ssid);
