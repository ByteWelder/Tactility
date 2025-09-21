#include <Tactility/Paths.h>

#include <Tactility/app/AppManifestParsing.h>
#include <Tactility/MountPoints.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>

#include <format>

namespace tt {

bool findFirstMountedSdCardPath(std::string& path) {
    // const auto sdcards = hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
    bool is_set = false;
    hal::findDevices<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard, [&is_set, &path](const auto& device) {
        if (device->isMounted()) {
            path = device->getMountPath();
            is_set = true;
            return false; // stop iterating
        } else {
            return true;
        }
    });
    return is_set;
}

std::string getSystemRootPath() {
    std::string root_path;
    if (!findFirstMountedSdCardPath(root_path)) {
        root_path = file::MOUNT_POINT_DATA;
    }
    return root_path;
}

std::string getTempPath() {
    return getSystemRootPath() + "/tmp";
}

std::string getAppInstallPath() {
    return getSystemRootPath() + "/app";
}

std::string getUserPath() {
    return getSystemRootPath() + "/user";
}

std::string getAppInstallPath(const std::string& appId) {
    assert(app::isValidId(appId));
    return std::format("{}/{}", getAppInstallPath(), appId);
}

std::string getAppUserPath(const std::string& appId) {
    assert(app::isValidId(appId));
    return std::format("{}/app/{}", getUserPath(), appId);
}

}