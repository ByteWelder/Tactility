#pragma once

#include "service.h"

#include "context.h"
#include "mutex.h"
#include "service_manifest.h"

typedef struct {
    FuriMutex* mutex;
    const ServiceManifest* manifest;
    void* data;
} ServiceData;

Service service_alloc(const ServiceManifest* _Nonnull manifest);
void service_free(Service _Nonnull service);
