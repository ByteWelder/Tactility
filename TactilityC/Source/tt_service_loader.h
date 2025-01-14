#pragma once

#include "tt_bundle.h"
#include "tt_app_context.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @param[in] id application manifest id
 * @param[in] bundle an allocated bundle (or NULL) of which the memory ownership is handed over to this function
 */
void tt_service_loader_start_app(const char* id, BundleHandle _Nullable bundle);
void tt_service_loader_stop_app();
AppContextHandle tt_service_loader_get_current_app();

#ifdef __cplusplus
}
#endif