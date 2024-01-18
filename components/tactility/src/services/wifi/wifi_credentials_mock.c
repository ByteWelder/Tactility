#ifndef ESP_PLATFORM

#include "wifi_credentials.h"

#include "log.h"

#define TAG "wifi_credentials"

// region Wi-Fi Credentials - public

bool tt_wifi_credentials_contains(const char* ssid) {
    // TODO: Implement
    return false;
}

void tt_wifi_credentials_init() {
    TT_LOG_W(TAG, "init mock implementation");
}

bool tt_wifi_credentials_get(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]) {
    // TODO: Implement
    return false;
}

bool tt_wifi_credentials_set(const char* ssid, char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT]) {
    // TODO: Implement
    return false;
}

bool tt_wifi_credentials_remove(const char* ssid) {
    // TODO: Implement
    return false;
}

// end region Wi-Fi Credentials - public

#endif