#pragma once

#include "service_manifest.h"
#include "tactility_core.h"

typedef void (*ServiceManifestCallback)(const ServiceManifest*, void* context);

void tt_service_registry_init();

void tt_service_registry_add(const ServiceManifest* manifest);
void tt_service_registry_remove(const ServiceManifest* manifest);
const ServiceManifest _Nullable* tt_service_registry_find_manifest_by_id(const std::string& id);
void tt_service_registry_for_each_manifest(ServiceManifestCallback callback, void* _Nullable context);

bool tt_service_registry_start(const std::string& id);
bool tt_service_registry_stop(const std::string& id);

Service _Nullable tt_service_find(const std::string& id);
