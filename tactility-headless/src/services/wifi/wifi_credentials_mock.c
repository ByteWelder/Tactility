#ifndef ESP_TARGET

#include "wifi_credentials.h"
#include "log.h"

#define TAG "wifi_credentials_mock"

static void hash_reset_all();

bool tt_wifi_credentials_contains(const char* ssid) {
    return false;
}

void tt_wifi_credentials_init() {
    TT_LOG_I(TAG, "init");
}

bool tt_wifi_credentials_get(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]) {
    return false;
}

bool tt_wifi_credentials_set(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]) {
    return false;
}

bool tt_wifi_credentials_remove(const char* ssid) {
    return false;
}

#endif // ESP_TARGET