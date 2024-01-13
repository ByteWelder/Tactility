#pragma once

#include "service_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef void (*ServiceManifestCallback)(const ServiceManifest*, void* context);

void service_registry_init();

void service_registry_add(const ServiceManifest* manifest);
void service_registry_remove(const ServiceManifest* manifest);
const ServiceManifest _Nullable* service_registry_find_manifest_by_id(const char* id);
void service_registry_for_each_manifest(ServiceManifestCallback callback, void* _Nullable context);

bool service_registry_start(const char* service_id);
bool service_registry_stop(const char* service_id);

#ifdef __cplusplus
}
#endif
