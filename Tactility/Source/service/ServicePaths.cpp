#include <Tactility/service/ServicePaths.h>

#include <Tactility/service/ServiceManifest.h>
#include <Tactility/MountPoints.h>

#define LVGL_PATH_PREFIX std::string("A:/")

#ifdef ESP_PLATFORM
#define PARTITION_PREFIX std::string("/")
#else
#define PARTITION_PREFIX std::string("")
#endif

namespace tt::service {

std::string ServicePaths::getDataDirectory() const {
    return PARTITION_PREFIX + file::DATA_PARTITION_NAME + "/service/" + manifest->id;
}

std::string ServicePaths::getDataPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + file::DATA_PARTITION_NAME + "/service/" + manifest->id + '/' + childPath;
}

std::string ServicePaths::getSystemDirectory() const {
    return PARTITION_PREFIX + file::SYSTEM_PARTITION_NAME + "/service/" + manifest->id;
}

std::string ServicePaths::getSystemPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + file::SYSTEM_PARTITION_NAME + "/service/" + manifest->id + '/' + childPath;
}

}
