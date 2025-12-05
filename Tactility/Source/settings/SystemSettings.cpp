#include <Tactility/MountPoints.h>
#include <Tactility/Mutex.h>
#include <Tactility/file/FileLock.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/settings/Language.h>
#include <Tactility/settings/SystemSettings.h>

#include <format>
#include <Tactility/file/File.h>

namespace tt::settings {

constexpr auto* TAG = "SystemSettings";
constexpr auto* FILE_PATH_FORMAT = "{}/settings/system.properties";

static Mutex mutex;
static bool cached = false;
static SystemSettings cachedSettings;

static bool loadSystemSettingsFromFile(SystemSettings& properties) {
    auto file_path = std::format(FILE_PATH_FORMAT, file::MOUNT_POINT_DATA);
    TT_LOG_I(TAG, "System settings loading from %s", file_path.c_str());
    std::map<std::string, std::string> map;
    if (!file::loadPropertiesFile(file_path, map)) {
        TT_LOG_E(TAG, "Failed to load %s", file_path.c_str());
        return false;
    }

    auto language_entry = map.find("language");
    if (language_entry != map.end()) {
        if (!fromString(language_entry->second, properties.language)) {
            TT_LOG_W(TAG, "Unknown language \"%s\" in %s", language_entry->second.c_str(), file_path.c_str());
            properties.language = Language::en_US;
        }
    } else {
        properties.language = Language::en_US;
    }

    auto time_format_entry = map.find("timeFormat24h");
    bool time_format_24h = time_format_entry == map.end() ? true : (time_format_entry->second == "true");
    properties.timeFormat24h = time_format_24h;

    TT_LOG_I(TAG, "System settings loaded");
    return true;
}

bool loadSystemSettings(SystemSettings& properties) {
    if (!cached) {
        if (!loadSystemSettingsFromFile(cachedSettings)) {
            return false;
        }
        cached = true;
    }

    properties = cachedSettings;
    return true;
}

bool saveSystemSettings(const SystemSettings& properties) {
    auto file_path = std::format(FILE_PATH_FORMAT, file::MOUNT_POINT_DATA);
    std::map<std::string, std::string> map;
    map["language"] = toString(properties.language);
    map["timeFormat24h"] = properties.timeFormat24h ? "true" : "false";

    if (!file::savePropertiesFile(file_path, map)) {
        TT_LOG_E(TAG, "Failed to save %s", file_path.c_str());
        return false;
    }

    cachedSettings = properties;
    cached = true;
    return true;
}

}