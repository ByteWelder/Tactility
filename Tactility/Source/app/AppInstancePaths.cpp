#include "app/AppInstancePaths.h"
#include "Partitions.h"

#define LVGL_PATH_PREFIX std::string("A:")

namespace tt::app {

std::string AppInstancePaths::getDataDirectory() const {
    return std::string(MOUNT_POINT_DATA) + "/data/" + manifest.id;
}

std::string AppInstancePaths::getDataDirectoryLvgl() const {
    return LVGL_PATH_PREFIX + getDataDirectory();
}

std::string AppInstancePaths::getDataPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return getDataDirectory() + '/' + childPath;
}

std::string AppInstancePaths::getDataPathLvgl(const std::string& childPath) const {
    return LVGL_PATH_PREFIX + getDataPath(childPath);
}

std::string AppInstancePaths::getSystemDirectory() const {
    return std::string(MOUNT_POINT_SYSTEM) + "/app/" + manifest.id;
}

std::string AppInstancePaths::getSystemDirectoryLvgl() const {
    return LVGL_PATH_PREFIX + getSystemDirectory();
}

std::string AppInstancePaths::getSystemPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return getSystemDirectory() + '/' + childPath;
}

std::string AppInstancePaths::getSystemPathLvgl(const std::string& childPath) const {
    return LVGL_PATH_PREFIX + getSystemPath(childPath);
}

}
