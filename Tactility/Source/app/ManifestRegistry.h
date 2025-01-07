#pragma once

#include "AppManifest.h"
#include <string>
#include <vector>

namespace tt::app {

/** Register an application with its manifest */
void addApp(const AppManifest* manifest);

/** Find an application manifest by its id
 * @param[in] id the manifest id
 * @return the application manifest if it was found
 */
const AppManifest _Nullable* findAppById(const std::string& id);

/** @return a list of all registered apps. This includes user and system apps. */
std::vector<const AppManifest*> getApps();

} // namespace
