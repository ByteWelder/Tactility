#ifndef ESP_TARGET

#include "WifiSettings.h"
#include "Log.h"

namespace tt::service::wifi::settings {

#define TAG "wifi_settings_mock"

bool contains(const char* ssid) {
    return false;
}

bool load(const char* ssid, WifiApSettings* settings) {
    return false;
}

bool save(const WifiApSettings* settings) {
    return true;
}

bool remove(const char* ssid) {
    return true;
}

} // namespace

#endif // ESP_TARGET