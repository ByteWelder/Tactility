#include <Tactility/service/wifi/WifiSettings.h>

#include <Tactility/file/File.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/Logger.h>
#include <Tactility/service/ServicePaths.h>
#include <Tactility/service/wifi/WifiPrivate.h>

namespace tt::service::wifi::settings {

static const auto LOGGER = Logger("WifiSettings");
constexpr auto* SETTINGS_KEY_ENABLE_ON_BOOT = "enableOnBoot";

struct WifiSettings {
    bool enableOnBoot;
};

static WifiSettings cachedSettings {
    .enableOnBoot = false
};

static bool cached = false;

static bool load(WifiSettings& settings) {
    auto service_context = findServiceContext();
    if (service_context == nullptr) {
        return false;
    }

    std::map<std::string, std::string> map;
    std::string settings_path = service_context->getPaths()->getUserDataPath("settings.properties");
    if (!file::loadPropertiesFile(settings_path, map)) {
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
    auto service_context = findServiceContext();
    if (service_context == nullptr) {
        return false;
    }
    std::map<std::string, std::string> map;
    map[SETTINGS_KEY_ENABLE_ON_BOOT] = settings.enableOnBoot ? "true" : "false";
    std::string settings_path = service_context->getPaths()->getUserDataPath("settings.properties");
    return file::savePropertiesFile(settings_path, map);
}

WifiSettings getCachedOrLoad() {
    if (!cached) {
        if (!load(cachedSettings)) {
            LOGGER.error("Failed to load");
        } else {
            cached = true;
        }
    }

    return cachedSettings;
}

void setEnableOnBoot(bool enable) {
    cachedSettings.enableOnBoot = enable;
    if (!save(cachedSettings)) {
        LOGGER.error("Failed to save");
    }
}

bool shouldEnableOnBoot() {
    return getCachedOrLoad().enableOnBoot;
}

} // namespace
