#include "Tactility/BootProperties.h"
#include "Tactility/MountPoints.h"
#include "Tactility/file/PropertiesFile.h"
#include "Tactility/hal/sdcard/SdCardDevice.h"

#include <Tactility/Log.h>
#include <Tactility/file/File.h>

#include <cassert>
#include <format>
#include <map>
#include <string>
#include <vector>

namespace tt {

constexpr auto* TAG = "BootProperties";
constexpr auto* PROPERTIES_FILE_FORMAT = "{}/boot.properties";
constexpr auto* PROPERTIES_KEY_LAUNCHER_APP_ID = "launcherAppId";
constexpr auto* PROPERTIES_KEY_AUTO_START_APP_ID = "autoStartAppId";

static std::string getPropertiesFilePath() {
    const auto sdcards = hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
    for (auto& sdcard : sdcards) {
        std::string path = std::format(PROPERTIES_FILE_FORMAT, sdcard->getMountPath());
        if (file::isFile(path)) {
            return path;
        }
    }
    return std::format(PROPERTIES_FILE_FORMAT, file::MOUNT_POINT_DATA);
}

bool loadBootProperties(BootProperties& properties) {
    const std::string path = getPropertiesFilePath();
    if (!file::loadPropertiesFile(path, [&properties](auto& key, auto& value) {
        if (key == PROPERTIES_KEY_AUTO_START_APP_ID) {
            properties.autoStartAppId = value;
        } else if (key == PROPERTIES_KEY_LAUNCHER_APP_ID) {
            properties.launcherAppId = value;
        }
    })) {
        TT_LOG_E(TAG, "Failed to load %s", path.c_str());
        return false;
    }

    return !properties.launcherAppId.empty();
}

bool saveBootProperties(const BootProperties& bootProperties) {
    assert(!bootProperties.launcherAppId.empty());
    const std::string path = getPropertiesFilePath();
    std::map<std::string, std::string> properties;
    properties[PROPERTIES_KEY_AUTO_START_APP_ID] = bootProperties.autoStartAppId;
    properties[PROPERTIES_KEY_LAUNCHER_APP_ID] = bootProperties.launcherAppId;
    return file::savePropertiesFile(path, properties);
}

}
