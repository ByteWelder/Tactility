#include "Tactility/BootProperties.h"

#include <Tactility/Log.h>
#include <Tactility/file/PropertiesFile.h>

#include <cassert>

namespace tt {

constexpr auto* TAG = "BootProperties";
constexpr auto* PROPERTIES_FILE = "/data/boot.properties";
constexpr auto* PROPERTIES_KEY_LAUNCHER_APP_ID = "launcherAppId";
constexpr auto* PROPERTIES_KEY_AUTO_START_APP_ID = "autoStartAppId";

bool loadBootProperties(BootProperties& properties) {
    if (!file::loadPropertiesFile(PROPERTIES_FILE, [&properties](auto& key, auto& value) {
        if (key == PROPERTIES_KEY_AUTO_START_APP_ID) {
            properties.autoStartAppId = value;
        } else if (key == PROPERTIES_KEY_LAUNCHER_APP_ID) {
            properties.launcherAppId = value;
        }
    })) {
        TT_LOG_E(TAG, "Failed to load %s", PROPERTIES_FILE);
        return false;
    }

    return !properties.launcherAppId.empty();
}

bool saveBootProperties(const BootProperties& bootProperties) {
    assert(!bootProperties.launcherAppId.empty());
    std::map<std::string, std::string> properties;
    properties[PROPERTIES_KEY_AUTO_START_APP_ID] = bootProperties.autoStartAppId;
    properties[PROPERTIES_KEY_LAUNCHER_APP_ID] = bootProperties.launcherAppId;
    return file::savePropertiesFile(PROPERTIES_FILE, properties);
}

}
