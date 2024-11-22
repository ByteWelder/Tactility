#pragma once

#include "app_manifest.h"
#include <string>

namespace tt {

typedef void (*AppManifestCallback)(const AppManifest*, void* context);

void app_manifest_registry_init();
void app_manifest_registry_add(const AppManifest* manifest);
void app_manifest_registry_remove(const AppManifest* manifest);
const AppManifest _Nullable* app_manifest_registry_find_by_id(const std::string& id);
void app_manifest_registry_for_each(AppManifestCallback callback, void* _Nullable context);
void app_manifest_registry_for_each_of_type(AppType type, void* _Nullable context, AppManifestCallback callback);

} // namespace
