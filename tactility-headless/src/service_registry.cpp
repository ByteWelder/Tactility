#include "service_registry.h"

#include "Mutex.h"
#include "service_i.h"
#include "service_manifest.h"
#include "tactility_core.h"
#include <string>
#include <unordered_map>

#define TAG "service_registry"

typedef std::unordered_map<std::string, const ServiceManifest*> ServiceManifestMap;
typedef std::unordered_map<std::string, ServiceData*> ServiceInstanceMap;

static ServiceManifestMap service_manifest_map;
static ServiceInstanceMap service_instance_map;

static Mutex* manifest_mutex = nullptr;
static Mutex* instance_mutex = nullptr;

void tt_service_registry_init() {
    tt_assert(manifest_mutex == nullptr);
    manifest_mutex = tt_mutex_alloc(MutexTypeNormal);

    tt_assert(instance_mutex == nullptr);
    instance_mutex = tt_mutex_alloc(MutexTypeNormal);
}

void service_registry_instance_lock() {
    tt_assert(instance_mutex != nullptr);
    tt_mutex_acquire(instance_mutex, TtWaitForever);
}

void service_registry_instance_unlock() {
    tt_assert(instance_mutex != nullptr);
    tt_mutex_release(instance_mutex);
}

void service_registry_manifest_lock() {
    tt_assert(manifest_mutex != nullptr);
    tt_mutex_acquire(manifest_mutex, TtWaitForever);
}

void service_registry_manifest_unlock() {
    tt_assert(manifest_mutex != nullptr);
    tt_mutex_release(manifest_mutex);
}
void tt_service_registry_add(const ServiceManifest* manifest) {
    TT_LOG_I(TAG, "adding %s", manifest->id);

    service_registry_manifest_lock();
    service_manifest_map[manifest->id] = manifest;
    service_registry_manifest_unlock();
}

const ServiceManifest* _Nullable tt_service_registry_find_manifest_by_id(const char* id) {
    service_registry_manifest_lock();
    auto iterator = service_manifest_map.find(id);
    _Nullable const ServiceManifest * manifest = iterator != service_manifest_map.end() ? iterator->second : nullptr;
    service_registry_manifest_unlock();
    return manifest;
}

static ServiceData* _Nullable service_registry_find_instance_by_id(const char* id) {
    service_registry_instance_lock();
    auto iterator = service_instance_map.find(id);
    _Nullable ServiceData* service = iterator != service_instance_map.end() ? iterator->second : nullptr;
    service_registry_instance_unlock();
    return service;
}

void tt_service_registry_for_each_manifest(ServiceManifestCallback callback, void* _Nullable context) {
    service_registry_manifest_lock();
    for (auto& it : service_manifest_map) {
        callback(it.second, context);
    }
    service_registry_manifest_unlock();
}

// TODO: return proper error/status instead of BOOL
bool tt_service_registry_start(const char* service_id) {
    TT_LOG_I(TAG, "starting %s", service_id);
    const ServiceManifest* manifest = tt_service_registry_find_manifest_by_id(service_id);
    if (manifest == nullptr) {
        TT_LOG_E(TAG, "manifest not found for service %s", service_id);
        return false;
    }

    ServiceData* service = tt_service_alloc(manifest);
    manifest->on_start(service);

    service_registry_instance_lock();
    service_instance_map[manifest->id] = service;
    service_registry_instance_unlock();
    TT_LOG_I(TAG, "started %s", service_id);

    return true;
}

Service _Nullable tt_service_find(const char* service_id) {
    return (Service)service_registry_find_instance_by_id(service_id);
}

bool tt_service_registry_stop(const char* service_id) {
    TT_LOG_I(TAG, "stopping %s", service_id);
    ServiceData* service = service_registry_find_instance_by_id(service_id);
    if (service == nullptr) {
        TT_LOG_W(TAG, "service not running: %s", service_id);
        return false;
    }

    service->manifest->on_stop(service);
    tt_service_free(service);

    service_registry_instance_lock();
    service_instance_map.erase(service_id);
    service_registry_instance_unlock();

    TT_LOG_I(TAG, "stopped %s", service_id);

    return true;
}
