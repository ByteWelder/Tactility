#pragma once

#include "app_manifest.h"
#include <string>

typedef void (*AppManifestCallback)(const AppManifest*, void* context);

void tt_app_manifest_registry_init();
void tt_app_manifest_registry_add(const AppManifest* manifest);
void tt_app_manifest_registry_remove(const AppManifest* manifest);
const AppManifest _Nullable* tt_app_manifest_registry_find_by_id(const std::string& id);
void tt_app_manifest_registry_for_each(AppManifestCallback callback, void* _Nullable context);
void tt_app_manifest_registry_for_each_of_type(AppType type, void* _Nullable context, AppManifestCallback callback);
