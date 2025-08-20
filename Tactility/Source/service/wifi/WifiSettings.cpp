#include "Tactility/service/wifi/WifiSettings.h"
#include "Tactility/Preferences.h"

#include <Tactility/LogEsp.h>
#include <Tactility/file/File.h>
#include <Tactility/file/PropertiesFile.h>

namespace tt::service::wifi::settings {

constexpr auto* TAG = "WifiSettings";
constexpr auto* SETTINGS_FILE = "/data/service/Wifi/wifi.properties";
constexpr auto* SETTINGS_KEY_ENABLE_ON_BOOT = "enableOnBoot";

struct WifiProperties {
    bool enableOnBoot;
};

static bool load(WifiProperties& properties) {
    std::map<std::string, std::string> map;
    if (!file::loadPropertiesFile(SETTINGS_FILE, map)) {
        return false;
    }

    if (!map.contains(SETTINGS_KEY_ENABLE_ON_BOOT)) {
        return false;
    }

    auto enable_on_boot_string = map[SETTINGS_KEY_ENABLE_ON_BOOT];
    properties.enableOnBoot = (enable_on_boot_string == "true");
    return true;
}

static bool save(const WifiProperties& properties) {
    std::map<std::string, std::string> map;
    map[SETTINGS_KEY_ENABLE_ON_BOOT] = properties.enableOnBoot ? "true" : "false";
    return file::savePropertiesFile(SETTINGS_FILE, map);
}

void setEnableOnBoot(bool enable) {
    WifiProperties properties { .enableOnBoot = enable };
    if (!save(properties)) {
        TT_LOG_E(TAG, "Failed to save %s", SETTINGS_FILE);
    }
}

bool shouldEnableOnBoot() {
    WifiProperties properties;
    if (!load(properties)) {
        return false;
    }
    return properties.enableOnBoot;
}

} // namespace
