#include "ManifestRegistry.h"
#include "Mutex.h"
#include "TactilityCore.h"
#include <unordered_map>

#define TAG "app_registry"

namespace tt::app {

typedef std::unordered_map<std::string, const Manifest*> AppManifestMap;

static AppManifestMap app_manifest_map;
static Mutex* hash_mutex = nullptr;

static void app_registry_lock() {
    tt_assert(hash_mutex != nullptr);
    tt_mutex_acquire(hash_mutex, TtWaitForever);
}

static void app_registry_unlock() {
    tt_assert(hash_mutex != nullptr);
    tt_mutex_release(hash_mutex);
}

void app_manifest_registry_init() {
    tt_assert(hash_mutex == nullptr);
    hash_mutex = tt_mutex_alloc(MutexTypeNormal);
}
void app_manifest_registry_add(const Manifest* manifest) {
    TT_LOG_I(TAG, "adding %s", manifest->id.c_str());

    app_registry_lock();
    app_manifest_map[manifest->id] = manifest;
    app_registry_unlock();
}

_Nullable const Manifest * app_manifest_registry_find_by_id(const std::string& id) {
    app_registry_lock();
    auto iterator = app_manifest_map.find(id);
    _Nullable const Manifest* result = iterator != app_manifest_map.end() ? iterator->second : nullptr;
    app_registry_unlock();
    return result;
}

std::vector<const Manifest*> app_manifest_registry_get() {
    std::vector<const Manifest*> manifests;
    app_registry_lock();
    for (const auto& item: app_manifest_map) {
        manifests.push_back(item.second);
    }
    app_registry_unlock();
    return manifests;
}

} // namespace
