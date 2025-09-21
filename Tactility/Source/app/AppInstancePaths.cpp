#include "Tactility/app/AppInstancePaths.h"

#include <Tactility/MountPoints.h>

#define LVGL_PATH_PREFIX std::string("A:/")
#ifdef ESP_PLATFORM
#define PARTITION_PREFIX std::string("/")
#else
#define PARTITION_PREFIX std::string("")
#endif

namespace tt::app {

std::string AppInstancePaths::getDataDirectory() const {
    if (manifest.appLocation.isInternal()) {

    }
    return PARTITION_PREFIX + file::DATA_PARTITION_NAME + "/app/" + manifest.appId;
}

std::string AppInstancePaths::getDataPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + file::DATA_PARTITION_NAME + "/app/" + manifest.appId + '/' + childPath;
}


std::string AppInstancePaths::getSystemDirectory() const {
    return PARTITION_PREFIX + file::SYSTEM_PARTITION_NAME + "/app/" + manifest.appId;
}

std::string AppInstancePaths::getSystemPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + file::SYSTEM_PARTITION_NAME + "/app/" + manifest.appId + '/' + childPath;
}

}
