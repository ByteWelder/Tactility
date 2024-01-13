#include "service_i.h"
#include "furi_core.h"
#include "log.h"

// region Alloc/free

Service service_alloc(const ServiceManifest* _Nonnull manifest) {
    ServiceData* data = malloc(sizeof(ServiceData));
    *data = (ServiceData) {
        .manifest = manifest,
        .mutex = furi_mutex_alloc(FuriMutexTypeRecursive),
        .data = NULL
    };
    return data;
}

void service_free(Service service) {
    ServiceData* data = (ServiceData*)service;
    furi_assert(service);
    furi_mutex_free(data->mutex);
    free(data);
}

// endregion Alloc/free

// region Internal

static void service_lock(ServiceData * data) {
    furi_mutex_acquire(data->mutex, FuriMutexTypeRecursive);
}

static void service_unlock(ServiceData* data) {
    furi_mutex_release(data->mutex);
}

// endregion Internal

// region Getters & Setters

const ServiceManifest* service_get_manifest(Service service) {
    ServiceData* data = (ServiceData*)service;
    service_lock(data);
    const ServiceManifest* manifest = data->manifest;
    service_unlock(data);
    return manifest;
}

void service_set_data(Service service, void* value) {
    ServiceData* data = (ServiceData*)service;
    service_lock(data);
    data->data = value;
    service_unlock(data);
}

void* _Nullable service_get_data(Service service) {
    ServiceData* data = (ServiceData*)service;
    service_lock(data);
    void* value = data->data;
    service_unlock(data);
    return value;
}

// endregion Getters & Setters