#include "ServiceRegistry.h"

#include "Mutex.h"
#include "Service.h"
#include "Manifest.h"
#include "TactilityCore.h"
#include <string>
#include <unordered_map>

namespace tt::service {

#define TAG "service_registry"

typedef std::unordered_map<std::string, const Manifest*> ManifestMap;
typedef std::unordered_map<std::string, Service*> ServiceInstanceMap;

static ManifestMap service_manifest_map;
static ServiceInstanceMap service_instance_map;

static Mutex manifest_mutex(MutexTypeNormal);
static Mutex instance_mutex(MutexTypeNormal);

void addService(const Manifest* manifest) {
    TT_LOG_I(TAG, "adding %s", manifest->id.c_str());

    manifest_mutex.acquire(TtWaitForever);
    service_manifest_map[manifest->id] = manifest;
    manifest_mutex.release();
}

const Manifest* _Nullable findManifestId(const std::string& id) {
    manifest_mutex.acquire(TtWaitForever);
    auto iterator = service_manifest_map.find(id);
    _Nullable const Manifest * manifest = iterator != service_manifest_map.end() ? iterator->second : nullptr;
    manifest_mutex.release();
    return manifest;
}

static Service* _Nullable service_registry_find_instance_by_id(const std::string& id) {
    manifest_mutex.acquire(TtWaitForever);
    auto iterator = service_instance_map.find(id);
    _Nullable Service* service = iterator != service_instance_map.end() ? iterator->second : nullptr;
    manifest_mutex.release();
    return service;
}

// TODO: return proper error/status instead of BOOL
bool startService(const std::string& id) {
    TT_LOG_I(TAG, "starting %s", id.c_str());
    const Manifest* manifest = findManifestId(id);
    if (manifest == nullptr) {
        TT_LOG_E(TAG, "manifest not found for service %s", id.c_str());
        return false;
    }

    auto* service = new Service(*manifest);
    manifest->onStart(*service);

    instance_mutex.acquire(TtWaitForever);
    service_instance_map[manifest->id] = service;
    instance_mutex.release();
    TT_LOG_I(TAG, "started %s", id.c_str());

    return true;
}

_Nullable Service* findServiceById(const std::string& service_id) {
    return (Service*)service_registry_find_instance_by_id(service_id);
}

bool stopService(const std::string& id) {
    TT_LOG_I(TAG, "stopping %s", id.c_str());
    Service* service = service_registry_find_instance_by_id(id);
    if (service == nullptr) {
        TT_LOG_W(TAG, "service not running: %s", id.c_str());
        return false;
    }

    service->getManifest().onStop(*service);
    delete service;

    instance_mutex.acquire(TtWaitForever);
    service_instance_map.erase(id);
    instance_mutex.release();

    TT_LOG_I(TAG, "stopped %s", id.c_str());

    return true;
}

} // namespace
