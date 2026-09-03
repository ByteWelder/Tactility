// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shared core behind every app_start*() (app/manager.h) and app_execute*()
 * (app/execute.h) entry point: deep-copies @a argv, allocates an instance id, installs
 * @a bindings into the new instance's fd table before app_scheduler_start() is called, then
 * starts it. @a manifest may be NULL for a location-based start with no manifest at all.
 * @retval ERROR_INVALID_ARGUMENT @a binding_count is nonzero but @a bindings is NULL
 * @retval ERROR_NOT_FOUND no AppLoaderApi is registered for @a location.type
 * @retval ERROR_OUT_OF_RANGE a binding's producer_fd is out of range
 * @retval ERROR_RESOURCE a binding's event_group has no free bits left to claim
 * @retval ERROR_NONE on success
 */
error_t app_manager_start_internal(const struct AppManifest* manifest, struct AppLocation location, struct AppStackConfig stack, AppInstanceId parent_instance_id, int argc, const char* const argv[], const struct AppStreamBinding* bindings, size_t binding_count, AppInstanceId* out_app_instance_id);

#ifdef __cplusplus
}
#endif
