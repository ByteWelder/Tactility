#include "AppManifestRegistry.h"
#include "Mutex.h"
#include "TactilityCore.h"
#include <unordered_map>

#define TAG "app_registry"

namespace tt {

typedef std::unordered_map<std::string, const AppManifest*> AppManifestMap;

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
void app_manifest_registry_add(const AppManifest* manifest) {
    TT_LOG_I(TAG, "adding %s", manifest->id.c_str());

    app_registry_lock();
    app_manifest_map[manifest->id] = manifest;
    app_registry_unlock();
}

_Nullable const AppManifest * app_manifest_registry_find_by_id(const std::string& id) {
    app_registry_lock();
    auto iterator = app_manifest_map.find(id);
    _Nullable const AppManifest* result = iterator != app_manifest_map.end() ? iterator->second : nullptr;
    app_registry_unlock();
    return result;
}

void app_manifest_registry_for_each_of_type(AppType type, void* _Nullable context, AppManifestCallback callback) {
    app_registry_lock();
    for (auto& it : app_manifest_map) {
        if (it.second->type == type) {
            callback(it.second, context);
        }
    }
    app_registry_unlock();
}

void app_manifest_registry_for_each(AppManifestCallback callback, void* _Nullable context) {
    app_registry_lock();
    for (auto& it : app_manifest_map) {
        callback(it.second, context);
    }
    app_registry_unlock();
}

} // namespace
