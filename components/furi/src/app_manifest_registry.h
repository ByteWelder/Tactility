#pragma once

#include "app_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const AppManifest* const INTERNAL_APP_MANIFESTS[];
extern const size_t INTERNAL_APP_COUNT;

typedef void (*AppManifestCallback)(const AppManifest*);

void app_manifest_registry_init();
void app_manifest_registry_add(const AppManifest _Nonnull* manifest);
void app_manifest_registry_remove(const AppManifest _Nonnull* manifest);
const AppManifest _Nullable* app_manifest_registry_find_by_id(const char* id);
void app_manifest_registry_for_each(AppManifestCallback callback);
void app_manifest_registry_for_each_of_type(AppType type, AppManifestCallback callback);

#ifdef __cplusplus
}
#endif
