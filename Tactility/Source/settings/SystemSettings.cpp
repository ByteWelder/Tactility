#include <Tactility/Mutex.h>
#include <Tactility/file/FileLock.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/settings/Language.h>
#include <Tactility/settings/SystemSettings.h>

namespace tt::settings {

constexpr auto* TAG = "SystemSettings";
constexpr auto* FILE_PATH = "/data/system.properties";

static Mutex mutex = Mutex();
static bool cached = false;
static SystemSettings cachedProperties;

static bool loadSystemSettingsFromFile(SystemSettings& properties) {
    std::map<std::string, std::string> map;
    if (!file::withLock<bool>(FILE_PATH, [&map] {
        return file::loadPropertiesFile(FILE_PATH, map);
    })) {
        TT_LOG_E(TAG, "Failed to load %s", FILE_PATH);
        return false;
    }

    auto language_entry = map.find("language");
    if (language_entry != map.end()) {
        if (!fromString(language_entry->second, properties.language)) {
            TT_LOG_W(TAG, "Unknown language \"%s\" in %s", language_entry->second.c_str(), FILE_PATH);
            properties.language = Language::en_US;
        }
    } else {
        properties.language = Language::en_US;
    }

    auto time_format_entry = map.find("timeFormat24h");
    bool time_format_24h = time_format_entry == map.end() ? true : (time_format_entry->second == "true");
    properties.timeFormat24h = time_format_24h;

    return true;
}

bool loadSystemSettings(SystemSettings& properties) {
    auto scoped_lock = mutex.asScopedLock();
    scoped_lock.lock();

    if (!cached) {
        if (!loadSystemSettingsFromFile(cachedProperties)) {
            return false;
        }
        cached = true;
    }

    properties = cachedProperties;
    return true;
}

bool saveSystemSettings(const SystemSettings& properties) {
    auto scoped_lock = mutex.asScopedLock();
    scoped_lock.lock();

    return file::withLock<bool>(FILE_PATH, [&properties] {
        std::map<std::string, std::string> map;
        map["language"] = toString(properties.language);
        map["timeFormat24h"] = properties.timeFormat24h ? "true" : "false";

        if (!file::savePropertiesFile(FILE_PATH, map)) {
            TT_LOG_E(TAG, "Failed to save %s", FILE_PATH);
            return false;
        }

        cachedProperties = properties;
        cached = true;
        return true;
    });
}

}