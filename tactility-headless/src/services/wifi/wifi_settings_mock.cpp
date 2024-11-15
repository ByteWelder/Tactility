#ifndef ESP_TARGET

#include "wifi_settings.h"
#include "log.h"

#define TAG "wifi_settings_mock"

bool tt_wifi_settings_contains(const char* ssid) {
    return false;
}

void tt_wifi_settings_init() {
    TT_LOG_I(TAG, "init");
}

bool tt_wifi_settings_load(const char* ssid, WifiApSettings* settings) {
    return false;
}

bool tt_wifi_settings_save(const WifiApSettings* settings) {
    return false;
}

bool tt_wifi_settings_remove(const char* ssid) {
    return false;
}

#endif // ESP_TARGET