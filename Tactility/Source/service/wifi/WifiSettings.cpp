#include "Tactility/service/wifi/WifiSettings.h"
#include "Tactility/Preferences.h"
#include "Tactility/file/PropertiesFile.h"

#include <Tactility/Log.h>
#include <Tactility/file/File.h>

namespace tt::service::wifi::settings {

constexpr auto* TAG = "WifiSettings";
constexpr auto* SETTINGS_FILE = "/data/settings/wifi.properties";
constexpr auto* SETTINGS_KEY_ENABLE_ON_BOOT = "enableOnBoot";

struct WifiSettings {
    bool enableOnBoot;
};

static bool load(WifiSettings& settings) {
    std::map<std::string, std::string> map;
    if (!file::loadPropertiesFile(SETTINGS_FILE, map)) {
        return false;
    }

    if (!map.contains(SETTINGS_KEY_ENABLE_ON_BOOT)) {
        return false;
    }

    auto enable_on_boot_string = map[SETTINGS_KEY_ENABLE_ON_BOOT];
    settings.enableOnBoot = (enable_on_boot_string == "true");
    return true;
}

static bool save(const WifiSettings& settings) {
    std::map<std::string, std::string> map;
    map[SETTINGS_KEY_ENABLE_ON_BOOT] = settings.enableOnBoot ? "true" : "false";
    return file::savePropertiesFile(SETTINGS_FILE, map);
}

void setEnableOnBoot(bool enable) {
    WifiSettings settings { .enableOnBoot = enable };
    if (!save(settings)) {
        TT_LOG_E(TAG, "Failed to save %s", SETTINGS_FILE);
    }
}

bool shouldEnableOnBoot() {
    WifiSettings settings;
    if (!load(settings)) {
        return false;
    }
    return settings.enableOnBoot;
}

} // namespace
