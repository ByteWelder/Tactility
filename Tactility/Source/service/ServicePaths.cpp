#include <Tactility/service/ServicePaths.h>

#include <Tactility/service/ServiceManifest.h>
#include <Tactility/MountPoints.h>

#include <cassert>
#include <format>

#ifdef ESP_PLATFORM
constexpr auto PARTITION_PREFIX = std::string("/");
#else
constexpr auto PARTITION_PREFIX = std::string("");
#endif

namespace tt::service {

std::string ServicePaths::getUserDataDirectory() const {
    return std::format("{}{}/service/{}", PARTITION_PREFIX, file::DATA_PARTITION_NAME, manifest->id);
}

std::string ServicePaths::getUserDataPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return std::format("{}/{}", getUserDataDirectory(), childPath);
}

std::string ServicePaths::getAssetsDirectory() const {
    return std::format("{}{}/service/{}/assets", PARTITION_PREFIX, file::SYSTEM_PARTITION_NAME, manifest->id);
}

std::string ServicePaths::getAssetsPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return std::format("{}/{}", getAssetsDirectory(), childPath);
}

}
