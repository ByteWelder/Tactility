#pragma once

#include <functional>
#include <map>
#include <string>

namespace tt::file {

bool loadPropertiesFile(const std::string& filePath, std::map<std::string, std::string>& outProperties);

bool loadPropertiesFile(const std::string& filePath, std::function<void(const std::string& key, const std::string& value)> callback);

bool savePropertiesFile(const std::string& filePath, const std::map<std::string, std::string>& properties);

}
