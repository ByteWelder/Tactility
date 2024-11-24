#pragma once

#include "AppManifest.h"
#include <string>
#include <vector>

namespace tt {

void app_manifest_registry_init();
void app_manifest_registry_add(const AppManifest* manifest);
void app_manifest_registry_remove(const AppManifest* manifest);
const AppManifest _Nullable* app_manifest_registry_find_by_id(const std::string& id);
std::vector<const AppManifest*> app_manifest_registry_get();

} // namespace
