#pragma once

#include "Manifest.h"
#include <string>
#include <vector>

namespace tt::app {

void app_manifest_registry_init();
void app_manifest_registry_add(const Manifest* manifest);
void app_manifest_registry_remove(const Manifest* manifest);
const Manifest _Nullable* app_manifest_registry_find_by_id(const std::string& id);
std::vector<const Manifest*> app_manifest_registry_get();

} // namespace
