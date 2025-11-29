#include <Tactility/MountPoints.h>
#include <Tactility/Mutex.h>
#include <Tactility/file/FileLock.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/settings/Language.h>
#include <Tactility/settings/SystemSettings.h>
#include <Tactility/settings/Time.h>

#include <format>
#include <Tactility/file/File.h>
#include <cstdio>
#include <string>

namespace tt::settings {

constexpr auto* TAG = "SystemSettings";
constexpr auto* FILE_PATH_FORMAT = "{}/settings/system.properties";

static Mutex mutex;
static bool cached = false;
static SystemSettings cachedSettings;

static bool parseEntry(const std::string& input, std::string& outName, std::string& outCode) {
    std::string partial_strip = input.substr(1, input.size() - 3);
    auto first_end_quote = partial_strip.find('"');
    if (first_end_quote == std::string::npos) {
        return false;
    } else {
        outName = partial_strip.substr(0, first_end_quote);
        outCode = partial_strip.substr(first_end_quote + 3);
        return true;
    }
}

static std::string findTimeZoneName(const std::string& code) {
    auto path = std::string(file::MOUNT_POINT_SYSTEM) + "/timezones.csv";
    auto* file = fopen(path.c_str(), "rb");
    if (file == nullptr) {
        TT_LOG_E(TAG, "Failed to open %s", path.c_str());
        return "";
    }
    char line[96];
    std::string name;
    std::string parsedCode;
    while (fgets(line, 96, file)) {
        if (parseEntry(line, name, parsedCode)) {
            if (parsedCode == code) {
                fclose(file);
                return name;
            }
        }
    }
    fclose(file);
    return "";
}

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

    // Allow system.properties to set the timezone on boot using timeZoneCode
    // If timeZoneCode is present and non-empty, derive the name and set it.
    auto tz_code_iter = map.find("timeZoneCode");
    if (tz_code_iter != map.end() && !tz_code_iter->second.empty()) {
        std::string tz_name = findTimeZoneName(tz_code_iter->second);
        if (!tz_name.empty()) {
            settings::setTimeZone(tz_name, tz_code_iter->second);
            properties.timeZoneCode = tz_code_iter->second;
        } else {
            TT_LOG_W(TAG, "Unknown timezone code \"%s\" in %s", tz_code_iter->second.c_str(), file_path.c_str());
            properties.timeZoneCode.clear();
        }
    } else {
        properties.timeZoneCode.clear();
    }

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
    if (!properties.timeZoneCode.empty()) {
        map["timeZoneCode"] = properties.timeZoneCode;
    }

    if (!file::savePropertiesFile(file_path, map)) {
        TT_LOG_E(TAG, "Failed to save %s", file_path.c_str());
        return false;
    }

    cachedSettings = properties;
    cached = true;
    return true;
}

}