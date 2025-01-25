#include "ManifestRegistry.h"
#include "Mutex.h"
#include "AppManifest.h"
#include <unordered_map>

#define TAG "app"

namespace tt::app {

typedef std::unordered_map<std::string, std::shared_ptr<AppManifest>> AppManifestMap;

static AppManifestMap app_manifest_map;
static Mutex hash_mutex(Mutex::Type::Normal);

void addApp(const AppManifest& manifest) {
    TT_LOG_I(TAG, "Registering manifest %s", manifest.id.c_str());

    hash_mutex.lock();

    if (!app_manifest_map.contains(manifest.id)) {
        app_manifest_map[manifest.id] = std::make_shared<AppManifest>(manifest);
    } else {
        TT_LOG_E(TAG, "App id in use: %s", manifest.id.c_str());
    }

    hash_mutex.unlock();
}

_Nullable std::shared_ptr<AppManifest> findAppById(const std::string& id) {
    hash_mutex.lock();
    auto result = app_manifest_map.find(id);
    hash_mutex.unlock();
    if (result != app_manifest_map.end()) {
        return result->second;
    } else {
        return nullptr;
    }
}

std::vector<std::shared_ptr<AppManifest>> getApps() {
    std::vector<std::shared_ptr<AppManifest>> manifests;
    hash_mutex.lock();
    for (const auto& item: app_manifest_map) {
        manifests.push_back(item.second);
    }
    hash_mutex.unlock();
    return manifests;
}

} // namespace
