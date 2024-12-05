#include "ManifestRegistry.h"
#include "Mutex.h"
#include "TactilityCore.h"
#include <unordered_map>

#define TAG "app"

namespace tt::app {

typedef std::unordered_map<std::string, const AppManifest*> AppManifestMap;

static AppManifestMap app_manifest_map;
static Mutex hash_mutex(MutexTypeNormal);

void addApp(const AppManifest* manifest) {
    TT_LOG_I(TAG, "Registering manifest %s", manifest->id.c_str());

    hash_mutex.acquire(TtWaitForever);
    app_manifest_map[manifest->id] = manifest;
    hash_mutex.release();
}

_Nullable const AppManifest * findAppById(const std::string& id) {
    hash_mutex.acquire(TtWaitForever);
    auto iterator = app_manifest_map.find(id);
    _Nullable const AppManifest* result = iterator != app_manifest_map.end() ? iterator->second : nullptr;
    hash_mutex.release();
    return result;
}

std::vector<const AppManifest*> getApps() {
    std::vector<const AppManifest*> manifests;
    hash_mutex.acquire(TtWaitForever);
    for (const auto& item: app_manifest_map) {
        manifests.push_back(item.second);
    }
    hash_mutex.release();
    return manifests;
}

} // namespace
