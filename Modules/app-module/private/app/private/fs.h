// SPDX-License-Identifier: Apache-2.0
#pragma once

// Minimal filesystem helpers shared by app-module internals.

#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#ifdef ESP_PLATFORM
#include <sys/unistd.h>
#else
#include <unistd.h>
#endif
#include <vector>

inline bool app_fs_is_directory(const std::string& path) {
    struct stat result {};
    return stat(path.c_str(), &result) == 0 && S_ISDIR(result.st_mode);
}

inline bool app_fs_is_file(const std::string& path) {
    struct stat result {};
    return stat(path.c_str(), &result) == 0 && S_ISREG(result.st_mode);
}

// Appends the full path of every direct subdirectory of @a path to @a out.
// No-op (not an error) if @a path can't be opened.
inline bool app_fs_delete_recursively(const std::string& path) {
    if (path.empty() || path == "/" || path == "." || path == "..") {
        return true;
    }

    // Use lstat() so symbolic links are not followed: a symlink that points at
    // an external directory must be removed as a leaf entry (unlink), not
    // recursed into.  app_fs_is_directory() uses stat() and would follow the
    // link, potentially deleting files outside the target tree.
    // ESP-IDF newlib has no lstat(); ESP32 filesystems (FAT/SPIFFS) don't
    // support symlinks, so stat() is equivalent there.
    struct stat st {};
#ifdef ESP_PLATFORM
    int rc = stat(path.c_str(), &st);
#else
    int rc = lstat(path.c_str(), &st);
#endif

    if (rc != 0) {
        return false;
    }

#ifndef ESP_PLATFORM
    if (S_ISLNK(st.st_mode)) {
        // Symlink — remove as a leaf regardless of its target.
        return unlink(path.c_str()) == 0;
    }
#endif

    if (S_ISDIR(st.st_mode)) {
        std::vector<std::string> children;

        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            return false;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            children.push_back(path + "/" + entry->d_name);
        }
        closedir(dir);

        bool success = true;
        for (const auto& child : children) {
            success = app_fs_delete_recursively(child);
            if (!success) {
                return false;
            }
        }

        return rmdir(path.c_str()) == 0;
    }

    // Regular file or other — unlink.
    return unlink(path.c_str()) == 0;
}

inline void app_fs_list_direct_subdirectories(const std::string& path, std::vector<std::string>& out) {
    std::vector<std::string> children;

    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        children.push_back(path + "/" + entry->d_name);
    }

    closedir(dir);

    for (const auto& child_path : children) {
        if (app_fs_is_directory(child_path)) {
            out.push_back(child_path);
        }
    }
}
