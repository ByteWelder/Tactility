// SPDX-License-Identifier: Apache-2.0
#pragma once

// Minimal filesystem helpers shared by app-module internals that need to look at on-disk app
// directories (app_install.cpp, manager.cpp's install-path scan) - app-module may not depend
// upward on Tactility::file, so this is a small local re-implementation (see
// app_metadata_parsing.cpp for the same constraint applied to properties-file loading).

#include <tactility/filesystem/file_mutex.h>

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
    FileMutex file_mutex;
    file_mutex_get(&file_mutex, path.c_str());
    file_mutex_lock(&file_mutex);
    auto is_dir = stat(path.c_str(), &result) == 0 && S_ISDIR(result.st_mode);
    file_mutex_unlock(&file_mutex);
    return is_dir;
}

inline bool app_fs_is_file(const std::string& path) {
    FileMutex file_mutex;
    file_mutex_get(&file_mutex, path.c_str());
    file_mutex_lock(&file_mutex);
    struct stat result {};
    auto retval = stat(path.c_str(), &result) == 0 && S_ISREG(result.st_mode);
    file_mutex_unlock(&file_mutex);
    return retval;
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
    FileMutex file_mutex;
    file_mutex_get(&file_mutex, path.c_str());
    file_mutex_lock(&file_mutex);
#ifdef ESP_PLATFORM
    int rc = stat(path.c_str(), &st);
#else
    int rc = lstat(path.c_str(), &st);
#endif
    file_mutex_unlock(&file_mutex);

    if (rc != 0) {
        return false;
    }

#ifndef ESP_PLATFORM
    if (S_ISLNK(st.st_mode)) {
        // Symlink — remove as a leaf regardless of its target.
        file_mutex_lock(&file_mutex);
        bool result = unlink(path.c_str()) == 0;
        file_mutex_unlock(&file_mutex);
        return result;
    }
#endif

    if (S_ISDIR(st.st_mode)) {
        // Collect child names while locked, then release before recursing —
        // child paths can resolve to the same mount mutex (see
        // app_fs_list_direct_subdirectories comment), so holding the parent
        // lock across the recursive call would self-deadlock.
        std::vector<std::string> children;

        file_mutex_lock(&file_mutex);
        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            file_mutex_unlock(&file_mutex);
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
        file_mutex_unlock(&file_mutex);

        bool success = true;
        for (const auto& child : children) {
            success = app_fs_delete_recursively(child);
            if (!success) {
                return false;
            }
        }

        file_mutex_lock(&file_mutex);
        bool result = rmdir(path.c_str()) == 0;
        file_mutex_unlock(&file_mutex);
        return result;
    }

    // Regular file or other — unlink.
    file_mutex_lock(&file_mutex);
    bool result = unlink(path.c_str()) == 0;
    file_mutex_unlock(&file_mutex);
    return result;
}

inline void app_fs_list_direct_subdirectories(const std::string& path, std::vector<std::string>& out) {
    // Collect child names while the directory lock is held, then release it before classifying
    // each one with app_fs_is_directory() - that function looks up and locks a FileMutex too,
    // and file_mutex_get() resolves a child path to the same registered mutex as its parent
    // mount. Calling it while still holding the directory's own lock would be a nested
    // acquisition of that same (possibly non-recursive) mutex, and could self-deadlock.
    std::vector<std::string> children;

    FileMutex file_mutex;
    file_mutex_get(&file_mutex, path.c_str());
    file_mutex_lock(&file_mutex);
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr) {
        file_mutex_unlock(&file_mutex);
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
    file_mutex_unlock(&file_mutex);

    for (const auto& child_path : children) {
        if (app_fs_is_directory(child_path)) {
            out.push_back(child_path);
        }
    }
}
