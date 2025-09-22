#include <Tactility/app/AppManifestParsing.h>

#include <regex>

namespace tt::app {

constexpr auto* TAG = "App";

static bool validateString(const std::string& value, const std::function<bool(const char)>& isValidChar) {
    for (const auto& c : value) {
        if (!isValidChar(c)) {
            return false;
        }
    }
    return true;
}

static bool getValueFromManifest(const std::map<std::string, std::string>& map, const std::string& key, std::string& output) {
    const auto iterator = map.find(key);
    if (iterator == map.end()) {
        TT_LOG_E(TAG, "Failed to find %s in manifest", key.c_str());
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
    return version.size() > 0 && validateString(version, [](const char c) {
        return std::isalnum(c) != 0 || c == '.';
    });
}

static bool isValidAppVersionName(const std::string& version) {
    return version.size() > 0 && validateString(version, [](const char c) {
        return std::isalnum(c) != 0 || c == '.' || c == '-' || c == '_';
    });
}

static bool isValidAppVersionCode(const std::string& version) {
    return version.size() > 0 && validateString(version, [](const char c) {
        return std::isdigit(c) != 0;
    });
}

static bool isValidName(const std::string& name) {
    return name.size() >= 2 && validateString(name, [](const char c) {
        return std::isalnum(c) != 0 || c == ' ' || c == '-';
    });
}

bool parseManifest(const std::map<std::string, std::string>& map, AppManifest& manifest) {
    TT_LOG_I(TAG, "Parsing manifest");

    // [manifest]

    std::string manifest_version;
    if (!getValueFromManifest(map, "[manifest]version", manifest_version)) {
        return false;
    }

    if (!isValidManifestVersion(manifest_version)) {
        TT_LOG_E(TAG, "Invalid version");
        return false;
    }

    // [app]

    if (!getValueFromManifest(map, "[app]id", manifest.appId)) {
        return false;
    }

    if (!isValidId(manifest.appId)) {
        TT_LOG_E(TAG, "Invalid app id");
        return false;
    }

    if (!getValueFromManifest(map, "[app]name", manifest.appName)) {
        return false;
    }

    if (!isValidName(manifest.appName)) {
        TT_LOG_I(TAG, "Invalid app name");
        return false;
    }

    if (!getValueFromManifest(map, "[app]versionName", manifest.appVersionName)) {
        return false;
    }

    if (!isValidAppVersionName(manifest.appVersionName)) {
        TT_LOG_E(TAG, "Invalid app version name");
        return false;
    }

    std::string version_code_string;
    if (!getValueFromManifest(map, "[app]versionCode", version_code_string)) {
        return false;
    }

    if (!isValidAppVersionCode(version_code_string)) {
        TT_LOG_E(TAG, "Invalid app version code");
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
