#pragma once

#include <stdbool.h>
#include "wifi_globals.h"

#ifdef __cplusplus
extern "C" {
#endif

void tt_wifi_credentials_init();

bool tt_wifi_credentials_contains(const char* ssid);

bool tt_wifi_credentials_load(const char* ssid, WifiApSettings* settings);

bool tt_wifi_credentials_save(const WifiApSettings* settings);

bool tt_wifi_credentials_remove(const char* ssid);

#ifdef __cplusplus
}
#endif
