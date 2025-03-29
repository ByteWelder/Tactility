#include "Tactility/service/ServiceInstancePaths.h"

#include "Tactility/Partitions.h"

#define LVGL_PATH_PREFIX std::string("A:/")

#ifdef ESP_PLATFORM
#define PARTITION_PREFIX std::string("/")
#else
#define PARTITION_PREFIX std::string("")
#endif

namespace tt::service {

std::string ServiceInstancePaths::getDataDirectory() const {
    return PARTITION_PREFIX + DATA_PARTITION_NAME + "/service/" + manifest->id;
}

std::string ServiceInstancePaths::getDataDirectoryLvgl() const {
    return LVGL_PATH_PREFIX + DATA_PARTITION_NAME + "/service/" + manifest->id;
}

std::string ServiceInstancePaths::getDataPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + DATA_PARTITION_NAME + "/service/" + manifest->id + '/' + childPath;
}

std::string ServiceInstancePaths::getDataPathLvgl(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return LVGL_PATH_PREFIX + DATA_PARTITION_NAME + "/service/" + manifest->id + '/' + childPath;
}

std::string ServiceInstancePaths::getSystemDirectory() const {
    return PARTITION_PREFIX + SYSTEM_PARTITION_NAME + "/service/" + manifest->id;
}

std::string ServiceInstancePaths::getSystemDirectoryLvgl() const {
    return LVGL_PATH_PREFIX + SYSTEM_PARTITION_NAME + "/service/" + manifest->id;
}

std::string ServiceInstancePaths::getSystemPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + SYSTEM_PARTITION_NAME + "/service/" + manifest->id + '/' + childPath;
}

std::string ServiceInstancePaths::getSystemPathLvgl(const std::string& childPath) const {
    return LVGL_PATH_PREFIX + SYSTEM_PARTITION_NAME + "/service/" + manifest->id + '/' + childPath;
}

}
