#pragma once

#include "service_manifest.h"
#include "TactilityCore.h"

namespace tt {

typedef void (*ServiceManifestCallback)(const ServiceManifest*, void* context);

void service_registry_init();

void service_registry_add(const ServiceManifest* manifest);
void service_registry_remove(const ServiceManifest* manifest);
_Nullable const ServiceManifest* service_registry_find_manifest_by_id(const std::string& id);
void service_registry_for_each_manifest(ServiceManifestCallback callback, void* _Nullable context);

bool service_registry_start(const std::string& id);
bool service_registry_stop(const std::string& id);

Service* _Nullable service_find(const std::string& id);

} // namespace
