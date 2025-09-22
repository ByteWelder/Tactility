#pragma once

#include <Tactility/app/AppManifest.h>

#include <map>
#include <string>

namespace tt::app {

bool isValidId(const std::string& id);

bool parseManifest(const std::map<std::string, std::string>& map, AppManifest& manifest);

}
