#ifndef ESP_TARGET

#include "wifi_credentials.h"
#include "log.h"

#define TAG "wifi_credentials_mock"

bool tt_wifi_credentials_contains(const char* ssid) {
    return false;
}

void tt_wifi_credentials_init() {
    TT_LOG_I(TAG, "init");
}

bool tt_wifi_credentials_load(const char* ssid, WifiApSettings* settings) {
    return false;
}

bool tt_wifi_credentials_save(const WifiApSettings* settings) {
    return false;
}

bool tt_wifi_credentials_remove(const char* ssid) {
    return false;
}

#endif // ESP_TARGET