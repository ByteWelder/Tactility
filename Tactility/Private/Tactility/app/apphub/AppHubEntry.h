#pragma once

#include <TactilityCpp/Allocator.h>

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

// The top-level entries buffer prefers PSRAM/SPIRAM via OptExternalAllocator; individual
// AppHubEntry string/vector members still use the default (internal-RAM) allocator.
using AppHubEntryList = std::vector<AppHubEntry, OptExternalAllocator<AppHubEntry>>;

bool parseJson(const std::string& filePath, AppHubEntryList& entries);

}