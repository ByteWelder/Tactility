#pragma once

#include "AppManifest.h"
#include <string>
#include <vector>

namespace tt::app {

void addApp(const AppManifest* manifest);
const AppManifest _Nullable* findAppById(const std::string& id);
std::vector<const AppManifest*> getApps();

} // namespace
