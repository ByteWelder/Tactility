// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/error.h>

#include <string>
#include <vector>

/**
 * Loads the kernel modules of an installed package. Each binary's main() returns the package's
 * `struct Module*` (see AppLoaderApi::get_module), which is then constructed, added and started.
 * All-or-nothing: when one module fails, the ones already started are stopped and unloaded again.
 * The binaries stay loaded until app_module_loader_unload_package().
 * @param[in] package_id the id the loaded modules are tracked under
 * @param[in] install_dir the package's install directory
 * @param[in] binaries the binary stems from the manifest's "module.N.binary" entries
 * @retval ERROR_NONE on success (also when @a binaries is empty)
 * @retval ERROR_INVALID_STATE modules of @a package_id are already loaded
 * @retval ERROR_NOT_SUPPORTED the path loader can't load modules
 */
error_t app_module_loader_load_package(const std::string& package_id, const std::string& install_dir, const std::vector<std::string>& binaries);

/**
 * Stops and unloads the modules of @a package_id in reverse order. A binary is only unloaded after its module
 * stopped, so a module that refuses to stop (e.g. because a driver of it is in use) stays running and registered,
 * together with the modules loaded before it.
 * @retval ERROR_NONE all modules were unloaded, or none were loaded
 * @retval ERROR_RESOURCE_BUSY a module is in use
 * @return otherwise the error of the module that failed to stop
 */
error_t app_module_loader_unload_package(const std::string& package_id);
