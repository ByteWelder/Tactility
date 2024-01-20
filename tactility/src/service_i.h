#pragma once

#include "service.h"

#include "mutex.h"
#include "service_manifest.h"

typedef struct {
    Mutex* mutex;
    const ServiceManifest* manifest;
    void* data;
} ServiceData;

ServiceData* tt_service_alloc(const ServiceManifest* manifest);
void tt_service_free(ServiceData* service);
