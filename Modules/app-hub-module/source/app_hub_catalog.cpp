// SPDX-License-Identifier: Apache-2.0
#include <app_hub/app_hub_catalog.h>

#include <app/package_manifest.h>

#include <tactility/log.h>

#include <cJSON.h>

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <format>
#include <memory>
#include <string_view>

constexpr auto* TAG = "app_hub_catalog";
constexpr auto* BASE_URL = "https://cdn.tactilityproject.org/apps";

static std::string get_version_without_postfix() {
    std::string version(TT_VERSION);
    auto index = version.find_first_of('-');
    return (index == std::string::npos) ? version : version.substr(0, index);
}

std::string app_hub_catalog_get_url() {
    return std::format("{}/{}/apps.json", BASE_URL, get_version_without_postfix());
}

std::string app_hub_catalog_get_download_url(const std::string& file) {
    return std::format("{}/{}/{}", BASE_URL, get_version_without_postfix(), file);
}

static bool read_string(const cJSON* object, const char* key, std::string& value) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsString(item)) {
        LOG_E(TAG, "%s is not a string", key);
        return false;
    }
    value = item->valuestring;
    return true;
}

static bool read_int32(const cJSON* object, const char* key, int32_t& value) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsNumber(item) || item->valuedouble != static_cast<double>(static_cast<int32_t>(item->valuedouble))) {
        LOG_E(TAG, "%s is not an integer", key);
        return false;
    }
    value = static_cast<int32_t>(item->valuedouble);
    return true;
}

static bool read_string_array(const cJSON* object, const char* key, std::vector<std::string>& values) {
    const cJSON* array = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsArray(array)) {
        LOG_E(TAG, "%s is not an array", key);
        return false;
    }
    values.clear();
    const cJSON* item;
    cJSON_ArrayForEach(item, array) {
        if (!cJSON_IsString(item)) {
            LOG_E(TAG, "%s has an item that is not a string", key);
            return false;
        }
        values.emplace_back(item->valuestring);
    }
    return true;
}

static bool parse_entry(const cJSON* object, AppHubCatalogEntry& entry) {
    // Optional: absent in apps.json files from before these fields existed
    if (cJSON_HasObjectItem(object, "requiresDeviceId") && !read_string_array(object, "requiresDeviceId", entry.requires_device_id)) {
        return false;
    }
    if (cJSON_HasObjectItem(object, "requiresRam") && !read_int32(object, "requiresRam", entry.requires_ram)) {
        return false;
    }
    return read_string(object, "appId", entry.id) &&
        read_string(object, "appVersionName", entry.version_name) &&
        read_int32(object, "appVersionCode", entry.version_code) &&
        read_string(object, "appName", entry.name) &&
        read_string(object, "appDescription", entry.description) &&
        read_string(object, "targetSdk", entry.target_sdk) &&
        read_string(object, "file", entry.file) &&
        read_string_array(object, "targetPlatforms", entry.target_platforms);
}

static std::unique_ptr<char[]> read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        return nullptr;
    }
    std::unique_ptr<char[]> data;
    if (fseek(file, 0, SEEK_END) == 0) {
        const long size = ftell(file);
        if (size >= 0 && fseek(file, 0, SEEK_SET) == 0) {
            data = std::make_unique<char[]>(size + 1);
            if (fread(data.get(), 1, size, file) == static_cast<size_t>(size)) {
                data[size] = '\0';
            } else {
                data = nullptr;
            }
        }
    }
    fclose(file);
    return data;
}

bool app_hub_catalog_parse(const char* path, AppHubCatalog& catalog) {
    auto data = read_file(path);
    if (data == nullptr) {
        LOG_E(TAG, "Failed to read %s", path);
        return false;
    }

    cJSON* json = cJSON_Parse(data.get());
    if (json == nullptr) {
        LOG_E(TAG, "Failed to parse %s", path);
        return false;
    }

    const cJSON* apps = cJSON_GetObjectItemCaseSensitive(json, "apps");
    if (!cJSON_IsArray(apps)) {
        LOG_E(TAG, "apps is not an array");
        cJSON_Delete(json);
        return false;
    }

    AppHubCatalog parsed;
    parsed.resize(cJSON_GetArraySize(apps));
    size_t index = 0;
    const cJSON* item;
    cJSON_ArrayForEach(item, apps) {
        if (!parse_entry(item, parsed[index++])) {
            LOG_E(TAG, "Failed to read entry");
            cJSON_Delete(json);
            return false;
        }
    }

    cJSON_Delete(json);
    catalog = std::move(parsed);
    return true;
}

bool app_hub_catalog_is_compatible(const AppHubCatalogEntry& entry) {
    // The simulator isn't a real MCU target, so it has no platform to match against
#ifdef ESP_PLATFORM
    if (!entry.target_platforms.empty() &&
        std::ranges::find(entry.target_platforms, std::string_view(CONFIG_IDF_TARGET)) == entry.target_platforms.end()) {
        return false;
    }
#endif

    // Same device and RAM rules as for installed packages
    PackageManifest package {};
    std::string device_ids;
    for (const auto& device_id : entry.requires_device_id) {
        if (!device_ids.empty()) {
            device_ids += ',';
        }
        device_ids += device_id;
    }
    if (device_ids.size() >= sizeof(package.requires_device_id)) {
        LOG_W(TAG, "%s: requiresDeviceId too long", entry.id.c_str());
        return false;
    }
    strcpy(package.requires_device_id, device_ids.c_str());
    if (entry.requires_ram < 0 || entry.requires_ram > UINT8_MAX) {
        LOG_W(TAG, "%s: invalid requiresRam %d", entry.id.c_str(), static_cast<int>(entry.requires_ram));
        return false;
    }
    package.requires_ram = static_cast<uint8_t>(entry.requires_ram);
    return app_package_manifest_is_compatible(&package);
}
