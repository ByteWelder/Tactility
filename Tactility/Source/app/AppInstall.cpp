#include <Tactility/app/App.h>

#include <minitar.h>

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <Tactility/file/File.h>
#include <Tactility/file/PropertiesFile.h>

constexpr auto* TAG = "App";

namespace tt::app {

static int untar_file(const minitar_entry* entry, const void* buf, const std::string& destinationPath) {
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

bool untar(const std::string& tarPath, const std::string& destinationPath) {
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
                    fprintf(stderr, "Failed to create directory %s/%s: %s\n", destinationPath.c_str(), entry.metadata.name, strerror(errno));
                    success = false;
                    break;
                }
            } else if (entry.metadata.type == MTAR_REGULAR) {
                auto ptr = static_cast<char*>(malloc(entry.metadata.size));
                if (!ptr) {
                    perror("malloc");
                    success = false;
                    break;
                }

                minitar_read_contents(&mp, &entry, ptr, entry.metadata.size);

                int status = untar_file(&entry, ptr, destinationPath);

                free(ptr);

                if (status != 0) {
                    fprintf(stderr, "Failed to extract file %s: %s\n", entry.metadata.path, strerror(errno));
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
                fprintf(stderr, "error: unknown entry type: %d", entry.metadata.type);
                success = false;
                break;
            }
        } else break;
    } while (true);
    minitar_close(&mp);
    return success;
}

bool install(const std::string& path) {
    auto filename = file::getLastPathSegment(path);
    const std::string target_path = std::format("/data/apps/{}", filename);
    if (file::isDirectory(target_path) && !file::deleteRecursively(target_path)) {
        TT_LOG_W(TAG, "Failed to delete %s", target_path.c_str());
    }

    if (!file::findOrCreateDirectory(target_path, 0777)) {
        TT_LOG_I(TAG, "Failed to create directory %s", target_path.c_str());
        return false;
    }

    TT_LOG_I(TAG, "Extracting app from %s to %s", path.c_str(), target_path.c_str());
    if (!untar(path, target_path)) {
        TT_LOG_E(TAG, "Failed to extract");
        return false;
    }

    auto manifest_path = target_path + "/manifest.properties";
    if (!file::isFile(manifest_path)) {
        TT_LOG_E(TAG, "Manifest not found at %s", manifest_path.c_str());
        return false;
    }

    std::map<std::string, std::string> properties;
    if (!file::loadPropertiesFile(manifest_path, properties)) {
        TT_LOG_E(TAG, "Failed to load manifest at %s", manifest_path.c_str());
        return false;
    }

    auto app_id_iterator = properties.find("[app]id");
    if (app_id_iterator == properties.end()) {
        TT_LOG_E(TAG, "Failed to find app id in manifest");
        return false;
    }

    const std::string renamed_target_path = std::format("/data/apps/{}", app_id_iterator->second);
    if (rename(target_path.c_str(), renamed_target_path.c_str()) != 0) {
        TT_LOG_E(TAG, "Failed to rename %s to %s", target_path.c_str(), app_id_iterator->second.c_str());
        return false;
    }

    return true;
}

} // namespace