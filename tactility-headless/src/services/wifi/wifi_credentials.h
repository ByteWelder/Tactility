#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TT_WIFI_CREDENTIALS_PASSWORD_LIMIT 64 // Should be equal to wifi_sta_config_t.password
// TODO: Move to config file
#define TT_WIFI_CREDENTIALS_LIMIT 16

void tt_wifi_credentials_init();

bool tt_wifi_credentials_contains(const char* ssid);

bool tt_wifi_credentials_get(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]);

bool tt_wifi_credentials_set(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]);

bool tt_wifi_credentials_remove(const char* ssid);

#ifdef __cplusplus
}
#endif
