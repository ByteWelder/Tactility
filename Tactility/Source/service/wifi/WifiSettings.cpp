#include "Tactility/service/wifi/WifiSettings.h"
#include "Tactility/Preferences.h"

#define WIFI_PREFERENCES_NAMESPACE "wifi"
#define WIFI_PREFERENCES_KEY_ENABLE_ON_BOOT "enable_on_boot"

namespace tt::service::wifi::settings {

// region General settings

void setEnableOnBoot(bool enable) {
    Preferences(WIFI_PREFERENCES_NAMESPACE).putBool(WIFI_PREFERENCES_KEY_ENABLE_ON_BOOT, enable);
}

bool shouldEnableOnBoot() {
    bool enable = false;
    Preferences(WIFI_PREFERENCES_NAMESPACE).optBool(WIFI_PREFERENCES_KEY_ENABLE_ON_BOOT, enable);
    return enable;
}

// endregion

// region AP

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

// endregion

} // namespace
