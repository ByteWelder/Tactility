#include "ServiceRegistry.h"

#include "Mutex.h"
#include "service/ServiceInstance.h"
#include "service/ServiceManifest.h"
#include "TactilityCore.h"
#include <string>
#include <unordered_map>

namespace tt::service {

#define TAG "service_registry"

typedef std::unordered_map<std::string, const ServiceManifest*> ManifestMap;
typedef std::unordered_map<std::string, ServiceInstance*> ServiceInstanceMap;

static ManifestMap service_manifest_map;
static ServiceInstanceMap service_instance_map;

static Mutex manifest_mutex(MutexTypeNormal);
static Mutex instance_mutex(MutexTypeNormal);

void addService(const ServiceManifest* manifest) {
    TT_LOG_I(TAG, "Adding %s", manifest->id.c_str());

    manifest_mutex.acquire(TtWaitForever);
    service_manifest_map[manifest->id] = manifest;
    manifest_mutex.release();
}

const ServiceManifest* _Nullable findManifestId(const std::string& id) {
    manifest_mutex.acquire(TtWaitForever);
    auto iterator = service_manifest_map.find(id);
    _Nullable const ServiceManifest * manifest = iterator != service_manifest_map.end() ? iterator->second : nullptr;
    manifest_mutex.release();
    return manifest;
}

static ServiceInstance* _Nullable service_registry_find_instance_by_id(const std::string& id) {
    manifest_mutex.acquire(TtWaitForever);
    auto iterator = service_instance_map.find(id);
    _Nullable ServiceInstance* service = iterator != service_instance_map.end() ? iterator->second : nullptr;
    manifest_mutex.release();
    return service;
}

// TODO: Return proper error/status instead of BOOL?
bool startService(const std::string& id) {
    TT_LOG_I(TAG, "Starting %s", id.c_str());
    const ServiceManifest* manifest = findManifestId(id);
    if (manifest == nullptr) {
        TT_LOG_E(TAG, "manifest not found for service %s", id.c_str());
        return false;
    }

    auto* service = new ServiceInstance(*manifest);
    manifest->onStart(*service);

    instance_mutex.acquire(TtWaitForever);
    service_instance_map[manifest->id] = service;
    instance_mutex.release();
    TT_LOG_I(TAG, "Started %s", id.c_str());

    return true;
}

_Nullable ServiceContext* findServiceById(const std::string& service_id) {
    return static_cast<ServiceInstance*>(service_registry_find_instance_by_id(service_id));
}

bool stopService(const std::string& id) {
    TT_LOG_I(TAG, "Stopping %s", id.c_str());
    ServiceInstance* service = service_registry_find_instance_by_id(id);
    if (service == nullptr) {
        TT_LOG_W(TAG, "service not running: %s", id.c_str());
        return false;
    }

    service->getManifest().onStop(*service);
    delete service;

    instance_mutex.acquire(TtWaitForever);
    service_instance_map.erase(id);
    instance_mutex.release();

    TT_LOG_I(TAG, "Stopped %s", id.c_str());

    return true;
}

} // namespace
