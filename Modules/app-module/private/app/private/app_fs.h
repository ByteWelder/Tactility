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

    if (app_fs_is_directory(path)) {
        FileMutex file_mutex;
        file_mutex_get(&file_mutex, path.c_str());
        file_mutex_lock(&file_mutex);

        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            file_mutex_unlock(&file_mutex);
            return false;
        }

        bool success = true;
        struct dirent* entry;
        while (success && (entry = readdir(dir)) != nullptr) {
            if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            success = app_fs_delete_recursively(path + "/" + entry->d_name);
        }
        closedir(dir);

        if (!success) {
            file_mutex_unlock(&file_mutex);
            return false;
        }

        bool result = rmdir(path.c_str()) == 0;
        file_mutex_unlock(&file_mutex);
        return result;
    }

    FileMutex file_mutex;
    file_mutex_get(&file_mutex, path.c_str());
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
