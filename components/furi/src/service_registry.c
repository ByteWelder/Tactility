#include "service_registry.h"

#include "furi_core.h"
#include "m-dict.h"
#include "m_cstr_dup.h"
#include "mutex.h"
#include "service_i.h"

#define TAG "service_registry"

DICT_DEF2(ServiceManifestDict, const char*, M_CSTR_DUP_OPLIST, const ServiceManifest*, M_PTR_OPLIST)
DICT_DEF2(ServiceInstanceDict, const char*, M_CSTR_DUP_OPLIST, const Service*, M_PTR_OPLIST)

#define APP_REGISTRY_FOR_EACH(manifest_var_name, code_to_execute)                                                               \
    {                                                                                                                           \
        service_registry_manifest_lock();                                                                                       \
        ServiceManifestDict_it_t it;                                                                                            \
        for (ServiceManifestDict_it(it, service_manifest_dict); !ServiceManifestDict_end_p(it); ServiceManifestDict_next(it)) { \
            const ServiceManifest*(manifest_var_name) = ServiceManifestDict_cref(it)->value;                                    \
            code_to_execute;                                                                                                    \
        }                                                                                                                       \
        service_registry_manifest_unlock();                                                                                     \
    }

ServiceManifestDict_t service_manifest_dict;
ServiceInstanceDict_t service_instance_dict;
FuriMutex* manifest_mutex = NULL;
FuriMutex* instance_mutex = NULL;

void service_registry_init() {
    furi_assert(manifest_mutex == NULL);
    manifest_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    ServiceManifestDict_init(service_manifest_dict);

    furi_assert(instance_mutex == NULL);
    instance_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    ServiceInstanceDict_init(service_instance_dict);
}

void service_registry_instance_lock() {
    furi_assert(instance_mutex != NULL);
    furi_mutex_acquire(instance_mutex, FuriWaitForever);
}

void service_registry_instance_unlock() {
    furi_assert(instance_mutex != NULL);
    furi_mutex_release(instance_mutex);
}

void service_registry_manifest_lock() {
    furi_assert(manifest_mutex != NULL);
    furi_mutex_acquire(manifest_mutex, FuriWaitForever);
}

void service_registry_manifest_unlock() {
    furi_assert(manifest_mutex != NULL);
    furi_mutex_release(manifest_mutex);
}
void service_registry_add(const ServiceManifest _Nonnull* manifest) {
    FURI_LOG_I(TAG, "adding %s", manifest->id);

    service_registry_manifest_lock();
    ServiceManifestDict_set_at(service_manifest_dict, manifest->id, manifest);
    service_registry_manifest_unlock();
}

void service_registry_remove(const ServiceManifest _Nonnull* manifest) {
    FURI_LOG_I(TAG, "removing %s", manifest->id);
    service_registry_manifest_lock();
    ServiceManifestDict_erase(service_manifest_dict, manifest->id);
    service_registry_manifest_unlock();
}

const ServiceManifest* _Nullable service_registry_find_manifest_by_id(const char* id) {
    service_registry_manifest_lock();
    const ServiceManifest** _Nullable manifest = ServiceManifestDict_get(service_manifest_dict, id);
    service_registry_manifest_unlock();
    return (manifest != NULL) ? *manifest : NULL;
}

Service* _Nullable service_registry_find_instance_by_id(const char* id) {
    service_registry_instance_lock();
    const Service** _Nullable service_ptr = ServiceInstanceDict_get(service_instance_dict, id);
    if (service_ptr == NULL) {
        return NULL;
    }
    Service* service = (Service*)*service_ptr;
    service_registry_instance_unlock();
    return service;
}

void service_registry_for_each_manifest(ServiceManifestCallback callback, void* _Nullable context) {
    APP_REGISTRY_FOR_EACH(manifest, {
        callback(manifest, context);
    });
}

// TODO: return proper error/status instead of BOOL
bool service_registry_start(const char* service_id) {
    FURI_LOG_I(TAG, "starting %s", service_id);
    const ServiceManifest* manifest = service_registry_find_manifest_by_id(service_id);
    if (manifest == NULL) {
        FURI_LOG_E(TAG, "manifest not found for service %s", service_id);
        return false;
    }

    Service* service = furi_service_alloc(manifest);
    service->manifest->on_start(&service->context);

    service_registry_instance_lock();
    ServiceInstanceDict_set_at(service_instance_dict, manifest->id, service);
    service_registry_instance_unlock();
    FURI_LOG_I(TAG, "started %s", service_id);

    return true;
}

bool service_registry_stop(const char* service_id) {
    FURI_LOG_I(TAG, "stopping %s", service_id);
    Service* service = service_registry_find_instance_by_id(service_id);
    if (service == NULL) {
        FURI_LOG_W(TAG, "service not running: %s", service_id);
        return false;
    }

    service->manifest->on_stop(&service->context);
    furi_service_free(service);

    service_registry_instance_lock();
    ServiceInstanceDict_erase(service_instance_dict, service_id);
    service_registry_instance_unlock();

    FURI_LOG_I(TAG, "stopped %s", service_id);

    return true;
}
