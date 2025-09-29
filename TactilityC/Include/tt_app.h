#pragma once

#include <cstdio>

#include "tt_app_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AppHandle;

/** @return the bundle that belongs to this application, or null if it wasn't started with parameters. */
BundleHandle _Nullable tt_app_get_parameters(AppHandle handle);

/**
 * Set the result before closing an app.
 * The result and bundle are passed along to the app that launched this app, when this app is closed.
 * @param[in] handle the app handle to set the result for
 * @param[in] result the result state to set
 * @param[in] bundle the result bundle to set
 */
void tt_app_set_result(AppHandle handle, AppResult result, BundleHandle _Nullable bundle);

/** @return true if a result was set for this app context */
bool tt_app_has_result(AppHandle handle);

/** Get the path to the user data directory for this app.
 * The app can store user-specific (mutable) data in there such as app settings.
 * @param[in] handle the app handle
 * @param[out] buffer the output buffer (recommended size is 256 bytes)
 * @param[inout] size used as input for maximum buffer size (including null terminator) and is set with the path string length by this function
 */
void tt_app_get_user_data_path(AppHandle handle, char* buffer, size_t* size);

/** Resolve a child path in the user directory of this app.
 * The app can store user-specific (mutable) data in there such as app settings.
 * @param[in] handle the app handle
 * @param[in] childPath the child path to resolve
 * @param[out] buffer the output buffer (recommended size is 256 bytes)
 * @param[inout] size used as input for maximum buffer size (including null terminator) and is set with the path string length by this function
 */
void tt_app_get_user_data_child_path(AppHandle handle, const char* childPath, char* buffer, size_t* size);

/** Get the path to the assets directory of this app.
 * The content in this path should be treated as read-only.
 * @param[in] handle the app handle
 * @param[out] buffer the output buffer (recommended size is 256 bytes)
 * @param[inout] size used as input for maximum buffer size (including null terminator) and is set with the path string length by this function
 */
void tt_app_get_assets_path(AppHandle handle, char* buffer, size_t* size);

/** Resolve a child path in the assets directory of this app.
 * The content in this path should be treated as read-only.
 * @param[in] handle the app handle
 * @param[in] childPath the child path to resolve
 * @param[out] buffer the output buffer (recommended size is 256 bytes)
 * @param[inout] size used as input for maximum buffer size (including null terminator) and is set with the path string length by this function
 */
void tt_app_get_assets_child_path(AppHandle handle, const char* childPath, char* buffer, size_t* size);

/**
 * Start an app by id.
 * @param[in] appId the app manifest id
 */
void tt_app_start(const char* appId);

/** Stop the currently running app */
void tt_app_stop();

/**
 * Start an app by id and bundle.
 * @param[in] appId the app manifest id
 * @param[in] parameters the parameters to pass onto the starting app
 */
void tt_app_start_with_bundle(const char* appId, BundleHandle parameters);

#ifdef __cplusplus
}
#endif