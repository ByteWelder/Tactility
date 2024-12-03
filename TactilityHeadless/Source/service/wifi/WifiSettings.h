#pragma once

#include "WifiGlobals.h"

namespace tt::service::wifi::settings {

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

bool contains(const char* ssid);

bool load(const char* ssid, WifiApSettings* settings);

bool save(const WifiApSettings* settings);

bool remove(const char* ssid);

void setEnableOnBoot(bool enable);

bool shouldEnableOnBoot();

} // namespace
