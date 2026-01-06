#include <Tactility/app/AppManifestParsing.h>

#include <Tactility/Logger.h>
#include <algorithm>
#include <regex>

namespace tt::app {

static const auto LOGGER = Logger("AppManifest");

constexpr bool validateString(const std::string& value, const std::function<bool(char)>& isValidChar) {
    return std::ranges::all_of(value, isValidChar);
}

static bool getValueFromManifest(const std::map<std::string, std::string>& map, const std::string& key, std::string& output) {
    const auto iterator = map.find(key);
    if (iterator == map.end()) {
        LOGGER.error("Failed to find {} in manifest", key);
        return false;
    }
    output = iterator->second;
    return true;
}

bool isValidId(const std::string& id) {
    return id.size() >= 5 && validateString(id, [](const char c) {
        return std::isalnum(c) != 0 || c == '.';
    });
}

static bool isValidManifestVersion(const std::string& version) {
    return !version.empty() && validateString(version, [](const char c) {
        return std::isalnum(c) != 0 || c == '.';
    });
}

static bool isValidAppVersionName(const std::string& version) {
    return !version.empty() && validateString(version, [](const char c) {
        return std::isalnum(c) != 0 || c == '.' || c == '-' || c == '_';
    });
}

static bool isValidAppVersionCode(const std::string& version) {
    return !version.empty() && validateString(version, [](const char c) {
        return std::isdigit(c) != 0;
    });
}

static bool isValidName(const std::string& name) {
    return name.size() >= 2 && validateString(name, [](const char c) {
        return std::isalnum(c) != 0 || c == ' ' || c == '-';
    });
}

bool parseManifest(const std::map<std::string, std::string>& map, AppManifest& manifest) {
    LOGGER.info("Parsing manifest");

    // [manifest]

    std::string manifest_version;
    if (!getValueFromManifest(map, "[manifest]version", manifest_version)) {
        return false;
    }

    if (!isValidManifestVersion(manifest_version)) {
        LOGGER.error("Invalid version");
        return false;
    }

    // [app]

    if (!getValueFromManifest(map, "[app]id", manifest.appId)) {
        return false;
    }

    if (!isValidId(manifest.appId)) {
        LOGGER.error("Invalid app id");
        return false;
    }

    if (!getValueFromManifest(map, "[app]name", manifest.appName)) {
        return false;
    }

    if (!isValidName(manifest.appName)) {
        LOGGER.error("Invalid app name");
        return false;
    }

    if (!getValueFromManifest(map, "[app]versionName", manifest.appVersionName)) {
        return false;
    }

    if (!isValidAppVersionName(manifest.appVersionName)) {
        LOGGER.error("Invalid app version name");
        return false;
    }

    std::string version_code_string;
    if (!getValueFromManifest(map, "[app]versionCode", version_code_string)) {
        return false;
    }

    if (!isValidAppVersionCode(version_code_string)) {
        LOGGER.error("Invalid app version code");
        return false;
    }

    manifest.appVersionCode = std::stoull(version_code_string);

    // [target]

    if (!getValueFromManifest(map, "[target]sdk", manifest.targetSdk)) {
        return false;
    }

    if (!getValueFromManifest(map, "[target]platforms", manifest.targetPlatforms)) {
        return false;
    }

    // Defaults

    manifest.appCategory = Category::User;
    manifest.appLocation = Location::external("");

    return true;
}

}
