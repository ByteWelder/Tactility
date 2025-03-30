#pragma once

#include "WifiGlobals.h"
#include <cstdint>

namespace tt::service::wifi::settings {

/**
 * This struct is stored as-is into NVS flash.
 *
 * The SSID and secret are increased by 1 byte to facilitate string null termination.
 * This makes it easier to use the char array as a string in various places.
 */
struct WifiApSettings {
    char ssid[TT_WIFI_SSID_LIMIT + 1] = { 0 };
    char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT + 1] = { 0 };
    int32_t channel = 0;
    bool auto_connect = true;
};

bool contains(const char* ssid);

bool load(const char* ssid, WifiApSettings* settings);

bool save(const WifiApSettings* settings);

bool remove(const char* ssid);

void setEnableOnBoot(bool enable);

bool shouldEnableOnBoot();

} // namespace
