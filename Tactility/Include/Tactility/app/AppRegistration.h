#pragma once

#include "App.h"
#include <string>
#include <vector>

namespace tt::app {

struct AppManifest;

/** Register an application with its manifest */
void addApp(const AppManifest& manifest);

/** Remove an app from the registry */
bool removeApp(const std::string& id);

/** Find an application manifest by its id
 * @param[in] id the manifest id
 * @return the application manifest if it was found
 */
_Nullable std::shared_ptr<AppManifest> findAppById(const std::string& id);

/** @return a list of all registered apps. This includes user and system apps. */
std::vector<std::shared_ptr<AppManifest>> getApps();

} // namespace
