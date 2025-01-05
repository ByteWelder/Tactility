#include "app/AppInstancePaths.h"
#include "Partitions.h"

#define LVGL_PATH_PREFIX std::string("A:/")
#ifdef ESP_PLATFORM
#define PARTITION_PREFIX std::string("/")
#else
#define PARTITION_PREFIX std::string("")
#endif

namespace tt::app {

std::string AppInstancePaths::getDataDirectory() const {
    return PARTITION_PREFIX + DATA_PARTITION_NAME + "/app/" + manifest.id;
}

std::string AppInstancePaths::getDataDirectoryLvgl() const {
    return LVGL_PATH_PREFIX + DATA_PARTITION_NAME + "/app/" + manifest.id;
}

std::string AppInstancePaths::getDataPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + DATA_PARTITION_NAME + "/app/" + manifest.id + '/' + childPath;
}

std::string AppInstancePaths::getDataPathLvgl(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return LVGL_PATH_PREFIX + DATA_PARTITION_NAME + "/app/" + manifest.id + '/' + childPath;
}

std::string AppInstancePaths::getSystemDirectory() const {
    return PARTITION_PREFIX + SYSTEM_PARTITION_NAME + "/app/" + manifest.id;
}

std::string AppInstancePaths::getSystemDirectoryLvgl() const {
    return LVGL_PATH_PREFIX + SYSTEM_PARTITION_NAME + "/app/" + manifest.id;
}

std::string AppInstancePaths::getSystemPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + SYSTEM_PARTITION_NAME + "/app/" + manifest.id + '/' + childPath;
}

std::string AppInstancePaths::getSystemPathLvgl(const std::string& childPath) const {
    return LVGL_PATH_PREFIX + SYSTEM_PARTITION_NAME + "/app/" + manifest.id + '/' + childPath;
}

}
