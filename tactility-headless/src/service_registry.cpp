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

static Mutex manifest_mutex(MutexTypeNormal);
static Mutex instance_mutex(MutexTypeNormal);

void tt_service_registry_add(const ServiceManifest* manifest) {
    TT_LOG_I(TAG, "adding %s", manifest->id.c_str());

    manifest_mutex.acquire(TtWaitForever);
    service_manifest_map[manifest->id] = manifest;
    manifest_mutex.release();
}

const ServiceManifest* _Nullable tt_service_registry_find_manifest_by_id(const std::string& id) {
    manifest_mutex.acquire(TtWaitForever);
    auto iterator = service_manifest_map.find(id);
    _Nullable const ServiceManifest * manifest = iterator != service_manifest_map.end() ? iterator->second : nullptr;
    manifest_mutex.release();
    return manifest;
}

static ServiceData* _Nullable service_registry_find_instance_by_id(const std::string& id) {
    manifest_mutex.acquire(TtWaitForever);
    auto iterator = service_instance_map.find(id);
    _Nullable ServiceData* service = iterator != service_instance_map.end() ? iterator->second : nullptr;
    manifest_mutex.release();
    return service;
}

void tt_service_registry_for_each_manifest(ServiceManifestCallback callback, void* _Nullable context) {
    manifest_mutex.acquire(TtWaitForever);
    for (auto& it : service_manifest_map) {
        callback(it.second, context);
    }
    manifest_mutex.release();
}

// TODO: return proper error/status instead of BOOL
bool tt_service_registry_start(const std::string& id) {
    TT_LOG_I(TAG, "starting %s", id.c_str());
    const ServiceManifest* manifest = tt_service_registry_find_manifest_by_id(id);
    if (manifest == nullptr) {
        TT_LOG_E(TAG, "manifest not found for service %s", id.c_str());
        return false;
    }

    ServiceData* service = tt_service_alloc(manifest);
    manifest->on_start(service);

    instance_mutex.acquire(TtWaitForever);
    service_instance_map[manifest->id] = service;
    instance_mutex.release();
    TT_LOG_I(TAG, "started %s", id.c_str());

    return true;
}

Service _Nullable tt_service_find(const std::string& service_id) {
    return (Service)service_registry_find_instance_by_id(service_id);
}

bool tt_service_registry_stop(const std::string& id) {
    TT_LOG_I(TAG, "stopping %s", id.c_str());
    ServiceData* service = service_registry_find_instance_by_id(id);
    if (service == nullptr) {
        TT_LOG_W(TAG, "service not running: %s", id.c_str());
        return false;
    }

    service->manifest->on_stop(service);
    tt_service_free(service);

    instance_mutex.acquire(TtWaitForever);
    service_instance_map.erase(id);
    instance_mutex.release();

    TT_LOG_I(TAG, "stopped %s", id.c_str());

    return true;
}
