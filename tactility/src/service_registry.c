#include "service_registry.h"

#include "m-dict.h"
#include "m_cstr_dup.h"
#include "mutex.h"
#include "service_i.h"
#include "service_manifest.h"
#include "tactility_core.h"

#define TAG "service_registry"

DICT_DEF2(ServiceManifestDict, const char*, M_CSTR_DUP_OPLIST, const ServiceManifest*, M_PTR_OPLIST)
DICT_DEF2(ServiceInstanceDict, const char*, M_CSTR_DUP_OPLIST, const ServiceData*, M_PTR_OPLIST)

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

static ServiceManifestDict_t service_manifest_dict;
static ServiceInstanceDict_t service_instance_dict;
static Mutex* manifest_mutex = NULL;
static Mutex* instance_mutex = NULL;

void tt_service_registry_init() {
    tt_assert(manifest_mutex == NULL);
    manifest_mutex = tt_mutex_alloc(MutexTypeNormal);
    ServiceManifestDict_init(service_manifest_dict);

    tt_assert(instance_mutex == NULL);
    instance_mutex = tt_mutex_alloc(MutexTypeNormal);
    ServiceInstanceDict_init(service_instance_dict);
}

void service_registry_instance_lock() {
    tt_assert(instance_mutex != NULL);
    tt_mutex_acquire(instance_mutex, TtWaitForever);
}

void service_registry_instance_unlock() {
    tt_assert(instance_mutex != NULL);
    tt_mutex_release(instance_mutex);
}

void service_registry_manifest_lock() {
    tt_assert(manifest_mutex != NULL);
    tt_mutex_acquire(manifest_mutex, TtWaitForever);
}

void service_registry_manifest_unlock() {
    tt_assert(manifest_mutex != NULL);
    tt_mutex_release(manifest_mutex);
}
void tt_service_registry_add(const ServiceManifest* manifest) {
    TT_LOG_I(TAG, "adding %s", manifest->id);

    service_registry_manifest_lock();
    ServiceManifestDict_set_at(service_manifest_dict, manifest->id, manifest);
    service_registry_manifest_unlock();
}

void tt_service_registry_remove(const ServiceManifest* manifest) {
    TT_LOG_I(TAG, "removing %s", manifest->id);
    service_registry_manifest_lock();
    ServiceManifestDict_erase(service_manifest_dict, manifest->id);
    service_registry_manifest_unlock();
}

const ServiceManifest* _Nullable tt_service_registry_find_manifest_by_id(const char* id) {
    service_registry_manifest_lock();
    const ServiceManifest** _Nullable manifest = ServiceManifestDict_get(service_manifest_dict, id);
    service_registry_manifest_unlock();
    return (manifest != NULL) ? *manifest : NULL;
}

ServiceData* _Nullable service_registry_find_instance_by_id(const char* id) {
    service_registry_instance_lock();
    const ServiceData** _Nullable service_ptr = ServiceInstanceDict_get(service_instance_dict, id);
    if (service_ptr == NULL) {
        return NULL;
    }
    ServiceData* service = (ServiceData*)*service_ptr;
    service_registry_instance_unlock();
    return service;
}

void tt_service_registry_for_each_manifest(ServiceManifestCallback callback, void* _Nullable context) {
    APP_REGISTRY_FOR_EACH(manifest, {
        callback(manifest, context);
    });
}

// TODO: return proper error/status instead of BOOL
bool tt_service_registry_start(const char* service_id) {
    TT_LOG_I(TAG, "starting %s", service_id);
    const ServiceManifest* manifest = tt_service_registry_find_manifest_by_id(service_id);
    if (manifest == NULL) {
        TT_LOG_E(TAG, "manifest not found for service %s", service_id);
        return false;
    }

    Service service = tt_service_alloc(manifest);
    manifest->on_start(service);

    service_registry_instance_lock();
    ServiceInstanceDict_set_at(service_instance_dict, manifest->id, service);
    service_registry_instance_unlock();
    TT_LOG_I(TAG, "started %s", service_id);

    return true;
}

bool tt_service_registry_stop(const char* service_id) {
    TT_LOG_I(TAG, "stopping %s", service_id);
    ServiceData* service = service_registry_find_instance_by_id(service_id);
    if (service == NULL) {
        TT_LOG_W(TAG, "service not running: %s", service_id);
        return false;
    }

    service->manifest->on_stop(service);
    tt_service_free(service);

    service_registry_instance_lock();
    ServiceInstanceDict_erase(service_instance_dict, service_id);
    service_registry_instance_unlock();

    TT_LOG_I(TAG, "stopped %s", service_id);

    return true;
}
