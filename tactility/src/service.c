#include "log.h"
#include "service_i.h"
#include "tactility_core.h"

// region Alloc/free

ServiceData* tt_service_alloc(const ServiceManifest* manifest) {
    ServiceData* data = malloc(sizeof(ServiceData));
    *data = (ServiceData) {
        .manifest = manifest,
        .mutex = tt_mutex_alloc(MutexTypeRecursive),
        .data = NULL
    };
    return data;
}

void tt_service_free(ServiceData* data) {
    tt_assert(data);
    tt_mutex_free(data->mutex);
    free(data);
}

// endregion Alloc/free

// region Internal

static void tt_service_lock(ServiceData * data) {
    tt_mutex_acquire(data->mutex, MutexTypeRecursive);
}

static void tt_service_unlock(ServiceData* data) {
    tt_mutex_release(data->mutex);
}

// endregion Internal

// region Getters & Setters

const ServiceManifest* tt_service_get_manifest(Service service) {
    ServiceData* data = (ServiceData*)service;
    tt_service_lock(data);
    const ServiceManifest* manifest = data->manifest;
    tt_service_unlock(data);
    return manifest;
}

void tt_service_set_data(Service service, void* value) {
    ServiceData* data = (ServiceData*)service;
    tt_service_lock(data);
    data->data = value;
    tt_service_unlock(data);
}

void* _Nullable tt_service_get_data(Service service) {
    ServiceData* data = (ServiceData*)service;
    tt_service_lock(data);
    void* value = data->data;
    tt_service_unlock(data);
    return value;
}

// endregion Getters & Setters