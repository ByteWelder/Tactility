#include <Tactility/app/App.h>

#include <Tactility/MountPoints.h>
#include <Tactility/app/AppManifest.h>
#include <Tactility/app/AppRegistration.h>
#include <Tactility/file/File.h>
#include <Tactility/file/FileLock.h>
#include <Tactility/file/PropertiesFile.h>
#include <Tactility/hal/Device.h>
#include <Tactility/hal/sdcard/SdCardDevice.h>

#include <errno.h>
#include <fcntl.h>
#include <format>
#include <libgen.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <minitar.h>

constexpr auto* TAG = "App";

namespace tt::app {

static int untarFile(const minitar_entry* entry, const void* buf, const std::string& destinationPath) {
    auto absolute_path = destinationPath + "/" + entry->metadata.path;
    if (!file::findOrCreateDirectory(destinationPath, 0777)) return 1;

    int fd = open(absolute_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0644);
    if (fd < 0) return 1;

    if (write(fd, buf, entry->metadata.size) < 0) return 1;

    // Note: fchmod() doesn't exist on ESP-IDF and chmod() does nothing on that platform
    if (chmod(absolute_path.c_str(), entry->metadata.mode) < 0) return 1;

    return close(fd);
}

static bool untar_directory(const minitar_entry* entry, const std::string& destinationPath) {
    auto absolute_path = destinationPath + "/" + entry->metadata.path;
    if (!file::findOrCreateDirectory(absolute_path, 0777)) return false;
    return true;
}

static bool untar(const std::string& tarPath, const std::string& destinationPath) {
    minitar mp;
    if (minitar_open(tarPath.c_str(), &mp) != 0) {
        perror(tarPath.c_str());
        return 1;
    }
    bool success = true;
    minitar_entry entry;

    do {
        if (minitar_read_entry(&mp, &entry) == 0) {
            TT_LOG_I(TAG, "Extracting %s", entry.metadata.path);
            if (entry.metadata.type == MTAR_DIRECTORY) {
                if (!strcmp(entry.metadata.name, ".") || !strcmp(entry.metadata.name, "..") || !strcmp(entry.metadata.name, "/")) continue;
                if (!untar_directory(&entry, destinationPath)) {
                    TT_LOG_E(TAG, "Failed to create directory %s/%s: %s", destinationPath.c_str(), entry.metadata.name, strerror(errno));
                    success = false;
                    break;
                }
            } else if (entry.metadata.type == MTAR_REGULAR) {
                auto file_buffer = static_cast<char*>(malloc(entry.metadata.size));
                if (!file_buffer) {
                    TT_LOG_E(TAG, "Failed to allocate %d bytes for file %s", entry.metadata.size, entry.metadata.path);;
                    success = false;
                    break;
                }

                minitar_read_contents(&mp, &entry, file_buffer, entry.metadata.size);
                int status = untarFile(&entry, file_buffer, destinationPath);
                free(file_buffer);
                if (status != 0) {
                    TT_LOG_E(TAG, "Failed to extract file %s: %s", entry.metadata.path, strerror(errno));
                    success = false;
                    break;
                }
            } else if (entry.metadata.type == MTAR_SYMLINK) {
                TT_LOG_E(TAG, "SYMLINK not supported");
            } else if (entry.metadata.type == MTAR_HARDLINK) {
                TT_LOG_E(TAG, "HARDLINK not supported");
            } else if (entry.metadata.type == MTAR_FIFO) {
                TT_LOG_E(TAG, "FIFO not supported");
            } else if (entry.metadata.type == MTAR_BLKDEV) {
                TT_LOG_E(TAG, "BLKDEV not supported");
            } else if (entry.metadata.type == MTAR_CHRDEV) {
                TT_LOG_E(TAG, "CHRDEV not supported");
            } else {
                TT_LOG_E(TAG, "Unknown entry type: %d", entry.metadata.type);
                success = false;
                break;
            }
        } else break;
    } while (true);
    minitar_close(&mp);
    return success;
}

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

std::string getTempPath() {
    std::string root_path;
    if (!findFirstMountedSdCardPath(root_path)) {
        root_path = file::MOUNT_POINT_DATA;
    }
    return root_path + "/tmp";
}

std::string getInstallPath() {
    std::string root_path;
    if (!findFirstMountedSdCardPath(root_path)) {
        root_path = file::MOUNT_POINT_DATA;
    }
    return root_path + "/apps";
}

bool install(const std::string& path) {
    // TODO: Make better: lock for each path type properly (source vs target)

    // We lock and unlock frequently because SPI SD card devices share
    // the lock with the display. We don't want to lock the display for very long.

    auto app_parent_path = getInstallPath();
    TT_LOG_I(TAG, "Installing app %s to %s", path.c_str(), app_parent_path.c_str());

    auto lock = file::getLock(app_parent_path)->asScopedLock();

    lock.lock();
    auto filename = file::getLastPathSegment(path);
    const std::string app_target_path = std::format("{}/{}", app_parent_path, filename);
    if (file::isDirectory(app_target_path) && !file::deleteRecursively(app_target_path)) {
        TT_LOG_W(TAG, "Failed to delete %s", app_target_path.c_str());
    }
    lock.unlock();

    lock.lock();
    if (!file::findOrCreateDirectory(app_target_path, 0777)) {
        TT_LOG_I(TAG, "Failed to create directory %s", app_target_path.c_str());
        return false;
    }
    lock.unlock();

    lock.lock();
    TT_LOG_I(TAG, "Extracting app from %s to %s", path.c_str(), app_target_path.c_str());
    if (!untar(path, app_target_path)) {
        TT_LOG_E(TAG, "Failed to extract");
        return false;
    }
    lock.unlock();

    lock.lock();
    auto manifest_path = app_target_path + "/manifest.properties";
    if (!file::isFile(manifest_path)) {
        TT_LOG_E(TAG, "Manifest not found at %s", manifest_path.c_str());
        return false;
    }
    lock.unlock();

    lock.lock();
    std::map<std::string, std::string> properties;
    if (!file::loadPropertiesFile(manifest_path, properties)) {
        TT_LOG_E(TAG, "Failed to load manifest at %s", manifest_path.c_str());
        return false;
    }
    lock.unlock();

    auto app_id_iterator = properties.find("[app]id");
    if (app_id_iterator == properties.end()) {
        TT_LOG_E(TAG, "Failed to find app id in manifest");
        return false;
    }

    auto app_name_entry = properties.find("[app]name");
    if (app_name_entry == properties.end()) {
        TT_LOG_E(TAG, "Failed to find app name in manifest");
        return false;
    }

    lock.lock();
    const std::string renamed_target_path = std::format("{}/{}", app_parent_path, app_id_iterator->second);
    if (file::isDirectory(renamed_target_path)) {
        if (!file::deleteRecursively(renamed_target_path)) {
            TT_LOG_W(TAG, "Failed to delete existing installation at %s", renamed_target_path.c_str());
            return false;
        }
    }
    lock.unlock();

    lock.lock();
    if (rename(app_target_path.c_str(), renamed_target_path.c_str()) != 0) {
        TT_LOG_E(TAG, "Failed to rename %s to %s", app_target_path.c_str(), app_id_iterator->second.c_str());
        return false;
    }
    lock.unlock();

    addApp({
        .id = app_id_iterator->second,
        .name = app_name_entry->second,
        .category = Category::User,
        .location = Location::external(renamed_target_path)
    });

    return true;
}

bool uninstall(const std::string& appId) {
    TT_LOG_I(TAG, "Uninstalling app %s", appId.c_str());
    auto app_path = getInstallPath() + "/" + appId;
    return file::withLock<bool>(app_path, [&app_path, &appId]() {
        if (!file::isDirectory(app_path)) {
            TT_LOG_E(TAG, "App %s not found at ", app_path.c_str());
            return false;
        }

        if (!file::deleteRecursively(app_path)) {
            return false;
        }

        if (!removeApp(appId)) {
            TT_LOG_W(TAG, "Failed to remove app %d from registry", appId.c_str());
        }

        return true;
    });
}

} // namespace