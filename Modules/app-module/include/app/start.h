// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/manager.h>

/**
 * This file contains functions to start and run apps that were registered to the app manager.
 * It differs from execute.h which runs executables from a specific path.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Starts @a id, a manifest already registered via app_manager_add(), passing @a argc/@a argv to
 * the new instance's own main function (see app/loader.h's AppMainFn), modelled on a C program's
 * main(argc, argv). For regular (non-modal) navigations that need to pass data to the target app
 * (e.g. "show details for this app id") without expecting a result back.
 * @param[in] argv @a argc strings; app-module makes its own deep copy before returning, so
 * @a argv and the strings it points to may be freed/go out of scope immediately after this call
 * returns (e.g. safe to pass a stack-local array of a caller's own std::string::c_str()s).
 * @retval ERROR_NOT_FOUND no manifest with this id is registered, or no AppLoaderApi is registered
 * @retval ERROR_NONE on success
 */
error_t app_start(const char* id, int argc, const char* const argv[], AppInstanceId* out_app_instance_id);

/**
 * Starts @a id as a child of @a parent_instance_id, for the purpose of receiving a result.
 *
 * When the child's task exits, an APP_EVENT_RESULT is delivered to @a parent_instance_id.
 * The result is whatever the child's AppMainFn/AppLoaderApi::run() returned, unless
 * @a parent_instance_id is 0, in which case no result is delivered (fire-and-forget, for
 * callers with no app_instance_id of their own). The parent is then responsible for calling
 * app_manager_stop() on the child's instance id to fully reap it.
 *
 * @param[in] argv @a argc strings; app-module makes its own deep copy before returning (same as
 * app_start()), so @a argv and the strings it points to may be
 * freed/go out of scope immediately after this call returns.
 * @retval ERROR_NOT_FOUND no manifest with this id is registered, or no AppLoaderApi is registered
 * @retval ERROR_NONE on success
 */
error_t app_start_for_result(const char* id, int argc, const char* const argv[], AppInstanceId parent_instance_id, AppInstanceId* out_app_instance_id);

/**
 * Same as app_start(), but installs @a bindings into the new instance's fd table before
 * its task begins executing (e.g. a child's stdio, piped through parent-owned AppStreams; see
 * app/stream.h). Writes the new instance's id into each bound stream's producer_id itself, since
 * the caller cannot know it in advance.
 * @param[in] bindings @a binding_count entries; each stream and buffer must stay alive (see
 * app_stream_subscribe()) until unsubscribed or the child exits.
 * @retval ERROR_NOT_FOUND no manifest with this id is registered, or no AppLoaderApi is registered
 * @retval ERROR_OUT_OF_RANGE a binding's producer_fd is out of range
 * @retval ERROR_RESOURCE a binding's event_group has no free bits left to claim
 * @retval ERROR_NONE on success
 */
error_t app_start_with_streams(const char* id, const struct AppStreamBinding* bindings, size_t binding_count, AppInstanceId* out_app_instance_id);

/**
 * Combines app_start_for_result() and app_start_with_streams(): starts @a id as
 * a modal child of @a parent_instance_id (see app_start_for_result()'s own doc for the
 * result-delivery contract) with @a bindings installed into its fd table before its task begins
 * executing (see app_start_with_streams()'s own doc for stream ownership).
 * For a child that needs to hand back more than an int32_t (e.g. a path) via its own stdout.
 * @param[in] argv see app_start_for_result().
 * @param[in] bindings see app_start_with_streams().
 * @retval ERROR_NOT_FOUND no manifest with this id is registered, or no AppLoaderApi is registered
 * @retval ERROR_OUT_OF_RANGE a binding's producer_fd is out of range
 * @retval ERROR_RESOURCE a binding's event_group has no free bits left to claim
 * @retval ERROR_NONE on success
 */
error_t app_start_for_result_with_streams(const char* id, int argc, const char* const argv[], const struct AppStreamBinding* bindings, size_t binding_count, AppInstanceId parent_instance_id, AppInstanceId* out_app_instance_id);

#ifdef __cplusplus
}
#endif
