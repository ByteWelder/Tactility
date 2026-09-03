// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stdbool.h>
#include <tactility/error.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Largest number of directories app_exec_path_add() can hold at once. */
#define APP_EXEC_MAX_PATHS 8

/**
 * Registers @a path as a directory whose contents may be executed. Applies recursively to
 * subdirectories. Registering the same path twice is a no-op.
 * @retval ERROR_OUT_OF_MEMORY the registry already holds APP_EXEC_MAX_PATHS entries
 * @retval ERROR_NONE on success (including when @a path was already registered)
 */
error_t app_exec_path_add(const char* path);

/**
 * Unregisters a directory previously passed to app_exec_path_add().
 * @retval ERROR_NOT_FOUND @a path was not registered
 * @retval ERROR_NONE on success
 */
error_t app_exec_path_remove(const char* path);

/**
 * True when @a file_path lives under a directory registered with app_exec_path_add(), at any
 * depth. Rejects any path containing a ".." segment rather than normalising it.
 */
bool app_exec_path_allowed(const char* file_path);

/**
 * Asks the registered APP_LOADER_PATH_SERVICE_ID loader (see app/loader.h) whether @a path is
 * runnable on this target, without loading it.
 * @return false if @a path can't be run, or if no path loader is registered
 */
bool app_exec_is_executable_path(const char* path);

#ifdef __cplusplus
}
#endif
