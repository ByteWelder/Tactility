#pragma once

#include <string>
#include <vector>

namespace tt::app::apphub {

struct AppHubEntry {
    std::string appId;
    std::string appVersionName;
    int32_t appVersionCode;
    std::string appName;
    std::string appDescription;
    std::string targetSdk;
    std::vector<std::string> targetPlatforms;
    std::string file;
};

bool parseJson(const std::string& filePath, std::vector<AppHubEntry>& entries);

}