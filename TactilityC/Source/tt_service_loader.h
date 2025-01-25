#pragma once

#include "tt_app.h"
#include "tt_bundle.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start an application providing a manifest id and an optional bundle.
 * Execution is always deferred.
 * This function generally returns immediately unless the scheduler is blocked.
 * @param[in] id application manifest id
 * @param[in] bundle an allocated bundle (or NULL) of which the memory ownership is handed over to this function
 */
void tt_service_loader_start_app(const char* id, BundleHandle _Nullable bundle);

/**
 * Stop the currently active app.
 * Execution is always deferred.
 * This function generally returns immediately unless the scheduler is blocked.
 */
void tt_service_loader_stop_app();

/**
 * Get the context handle of the app that is currently shown on the screen.
 */
AppHandle tt_service_loader_get_current_app();

#ifdef __cplusplus
}
#endif