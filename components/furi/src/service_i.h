#pragma once

#include "service_manifest.h"
#include "context.h"

typedef struct {
    const ServiceManifest* manifest;
    Context context;
} Service;

Service* furi_service_alloc(const ServiceManifest* _Nonnull manifest);
void furi_service_free(Service* _Nonnull service);
