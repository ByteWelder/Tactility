#pragma once

#include "tt_app_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AppContextHandle;

/** @return the data that was attached to this app context */
void* _Nullable tt_app_context_get_data(AppContextHandle handle);

/**
 * Attach data to an application context.
 * Don't forget to manually delete allocated memory when onStopped() is called.
 * @param[in] handle the app context handle
 * @param[in] data the data to attach
 */
void tt_app_context_set_data(AppContextHandle handle, void* _Nullable data);

/** @return the bundle that belongs to this application, or null */
BundleHandle _Nullable tt_app_context_get_parameters(AppContextHandle handle);

/**
 * Set the result before closing an app.
 * The result and bundle are passed along to the app that launched this app, when this app is closed.
 * @param[in] handle the app context handle to set the result for
 * @param[in] result the result state to set
 * @param[in] bundle the result bundle to set
 */
void tt_app_context_set_result(AppContextHandle handle, Result result, BundleHandle _Nullable bundle);

/** @return true if a result was set for this app context */
bool tt_app_context_has_result(AppContextHandle handle);

#ifdef __cplusplus
}
#endif