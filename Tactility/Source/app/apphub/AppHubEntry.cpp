#include <Tactility/app/apphub/AppHubEntry.h>
#include <Tactility/file/File.h>
#include <Tactility/json/Reader.h>
#include <Tactility/Logger.h>

namespace tt::app::apphub {

static const auto LOGGER = Logger("AppHubJson");

static bool parseEntry(const cJSON* object, AppHubEntry& entry) {
    const json::Reader reader(object);
    return reader.readString("appId", entry.appId) &&
         reader.readString("appVersionName", entry.appVersionName) &&
         reader.readInt32("appVersionCode", entry.appVersionCode) &&
         reader.readString("appName", entry.appName) &&
         reader.readString("appDescription", entry.appDescription) &&
         reader.readString("targetSdk", entry.targetSdk) &&
         reader.readString("file", entry.file) &&
         reader.readStringArray("targetPlatforms", entry.targetPlatforms);
}

bool parseJson(const std::string& filePath, std::vector<AppHubEntry>& entries) {
    auto lock = file::getLock(filePath)->asScopedLock();
    lock.lock();

    auto data = file::readString(filePath);
    if (data == nullptr) {
        LOGGER.error("Failed to read {}", filePath);
        return false;
    }

    auto data_ptr = reinterpret_cast<const char*>(data.get());
    auto* json = cJSON_Parse(data_ptr);
    if (json == nullptr) {
        LOGGER.error("Failed to parse {}", filePath);
        return false;
    }

    const cJSON* apps_json = cJSON_GetObjectItemCaseSensitive(json, "apps");
    if (!cJSON_IsArray(apps_json)) {
        cJSON_Delete(json);
        LOGGER.error("apps is not an array");
        return false;
    }

    auto apps_size = cJSON_GetArraySize(apps_json);
    entries.resize(apps_size);
    for (int i = 0; i < apps_size; ++i) {
        auto& entry = entries.at(i);
        auto* entry_json = cJSON_GetArrayItem(apps_json, i);
        if (!parseEntry(entry_json, entry)) {
            LOGGER.error("Failed to read entry");
            cJSON_Delete(json);
            return false;
        }
    }

    cJSON_Delete(json);
    return true;
}

}
