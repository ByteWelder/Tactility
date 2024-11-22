#ifndef ESP_TARGET

#include "wifi_settings.h"
#include "log.h"

namespace tt::service::wifi::settings {

#define TAG "wifi_settings_mock"

bool contains(const char* ssid) {
    return false;
}

bool load(const char* ssid, WifiApSettings* settings) {
    return false;
}

bool save(const WifiApSettings* settings) {
    return false;
}

bool remove(const char* ssid) {
    return false;
}

} // namespace

#endif // ESP_TARGET