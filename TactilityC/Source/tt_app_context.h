#pragma once

#include "tt_app_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AppContextHandle;

void* _Nullable tt_app_context_get_data(AppContextHandle handle);
void tt_app_context_set_data(AppContextHandle handle, void* _Nullable data);
BundleHandle _Nullable tt_app_context_get_parameters(AppContextHandle handle);
void tt_app_context_set_result(AppContextHandle handle, Result result, BundleHandle _Nullable bundle);
bool tt_app_context_has_result(AppContextHandle handle);

#ifdef __cplusplus
}
#endif