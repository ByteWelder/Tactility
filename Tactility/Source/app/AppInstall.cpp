#include "Tactility/Paths.h"

#include <Tactility/app/App.h>
#include <Tactility/app/AppManifestParsing.h>

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

void cleanupInstallDirectory(const std::string& path) {
    if (!file::deleteRecursively(path)) {
        TT_LOG_W(TAG, "Failed to delete existing installation at %s", path.c_str());
    }
}

bool install(const std::string& path) {
    // We lock and unlock frequently because SPI SD card devices share
    // the lock with the display. We don't want to lock the display for very long.

    auto app_parent_path = getAppInstallPath();
    TT_LOG_I(TAG, "Installing app %s to %s", path.c_str(), app_parent_path.c_str());

    auto filename = file::getLastPathSegment(path);
    const std::string app_target_path = std::format("{}/{}", app_parent_path, filename);
    if (file::isDirectory(app_target_path) && !file::deleteRecursively(app_target_path)) {
        TT_LOG_W(TAG, "Failed to delete %s", app_target_path.c_str());
    }

    if (!file::findOrCreateDirectory(app_target_path, 0777)) {
        TT_LOG_I(TAG, "Failed to create directory %s", app_target_path.c_str());
        return false;
    }

    auto target_path_lock = file::getLock(app_parent_path)->asScopedLock();
    auto source_path_lock = file::getLock(path)->asScopedLock();
    target_path_lock.lock();
    source_path_lock.lock();
    TT_LOG_I(TAG, "Extracting app from %s to %s", path.c_str(), app_target_path.c_str());
    if (!untar(path, app_target_path)) {
        TT_LOG_E(TAG, "Failed to extract");
        return false;
    }
    source_path_lock.unlock();
    target_path_lock.unlock();

    auto manifest_path = app_target_path + "/manifest.properties";
    if (!file::isFile(manifest_path)) {
        TT_LOG_E(TAG, "Manifest not found at %s", manifest_path.c_str());
        cleanupInstallDirectory(app_target_path);
        return false;
    }

    std::map<std::string, std::string> properties;
    if (!file::loadPropertiesFile(manifest_path, properties)) {
        TT_LOG_E(TAG, "Failed to load manifest at %s", manifest_path.c_str());
        cleanupInstallDirectory(app_target_path);
        return false;
    }

    AppManifest manifest;
    if (!parseManifest(properties, manifest)) {
        TT_LOG_W(TAG, "Invalid manifest");
        cleanupInstallDirectory(app_target_path);
        return false;
    }

    // If the app was already running, then stop it
    if (isRunning(manifest.appId)) {
        stopAll(manifest.appId);
    }

    const std::string renamed_target_path = std::format("{}/{}", app_parent_path, manifest.appId);
    if (file::isDirectory(renamed_target_path)) {
        if (!file::deleteRecursively(renamed_target_path)) {
            TT_LOG_W(TAG, "Failed to delete existing installation at %s", renamed_target_path.c_str());
            cleanupInstallDirectory(app_target_path);
            return false;
        }
    }

    target_path_lock.lock();
    bool rename_success = rename(app_target_path.c_str(), renamed_target_path.c_str()) == 0;
    target_path_lock.unlock();

    if (!rename_success) {
        TT_LOG_E(TAG, "Failed to rename \"%s\" to \"%s\"", app_target_path.c_str(), manifest.appId.c_str());
        cleanupInstallDirectory(app_target_path);
        return false;
    }

    manifest.appLocation = Location::external(renamed_target_path);

    addApp(manifest);

    return true;
}

bool uninstall(const std::string& appId) {
    TT_LOG_I(TAG, "Uninstalling app %s", appId.c_str());

    // If the app was running, then stop it
    if (isRunning(appId)) {
        stopAll(appId);
    }

    auto app_path = getAppInstallPath(appId);
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
}

} // namespace