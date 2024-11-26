#pragma once

#include "Manifest.h"
#include <string>
#include <vector>

namespace tt::app {

void addApp(const Manifest* manifest);
const Manifest _Nullable* findAppById(const std::string& id);
std::vector<const Manifest*> getApps();

} // namespace
