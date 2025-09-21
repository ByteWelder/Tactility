#include <Tactility/app/AppPaths.h>

#include <Tactility/app/AppManifest.h>
#include <Tactility/MountPoints.h>

#define LVGL_PATH_PREFIX std::string("A:/")
#ifdef ESP_PLATFORM
#define PARTITION_PREFIX std::string("/")
#else
#define PARTITION_PREFIX std::string("")
#endif

namespace tt::app {

std::string AppPaths::getDataDirectory() const {
    if (manifest.appLocation.isInternal()) {

    }
    return PARTITION_PREFIX + file::DATA_PARTITION_NAME + "/app/" + manifest.appId;
}

std::string AppPaths::getDataPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + file::DATA_PARTITION_NAME + "/app/" + manifest.appId + '/' + childPath;
}


std::string AppPaths::getSystemDirectory() const {
    return PARTITION_PREFIX + file::SYSTEM_PARTITION_NAME + "/app/" + manifest.appId;
}

std::string AppPaths::getSystemPath(const std::string& childPath) const {
    assert(!childPath.starts_with('/'));
    return PARTITION_PREFIX + file::SYSTEM_PARTITION_NAME + "/app/" + manifest.appId + '/' + childPath;
}

}
