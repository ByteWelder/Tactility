#pragma once

#include <string>

namespace tt {

bool findFirstMountedSdCardPath(std::string& path);

std::string getSystemRootPath();

std::string getTempPath();

std::string getAppInstallPath();

std::string getAppInstallPath(const std::string& appId);

std::string getUserPath();

std::string getAppUserPath(const std::string& appId);

}
