// SPDX-License-Identifier: Apache-2.0
#include <tactility/filesystem/file_mutex.h>

#include <cstring>
#include <string>
#include <vector>

static const FileMutex no_mutex = {
    .lock = nullptr,
    .try_lock = nullptr,
    .unlock = nullptr,
};

struct FileMutexEntry {
    std::string path;
    FileMutex mutex;
};

static std::vector<FileMutexEntry> mutex_entries;

extern "C" {

void file_mutex_register(const FileMutex* mutex, const char* path) {
    // Skip if entry for path exists
    for (auto& entry : mutex_entries) {
        if (entry.path == path) {
            return;
        }
    }

    // Store a copy of the entry
    mutex_entries.push_back({
        .path = path,
        .mutex = *mutex
    });
}

void file_mutex_get(FileMutex* mutex, const char* path) {
    std::string path_string = path;
    for (auto& entry : mutex_entries) {
        // Match the mount path itself, or a descendant (e.g. "/sdcard" registered, "/sdcard/config.json" requested).
        bool is_match = path_string == entry.path ||
            (entry.path == "/" && !path_string.empty() && path_string[0] == '/') ||
            (path_string.rfind(entry.path, 0) == 0 && path_string[entry.path.size()] == '/');
        if (is_match) {
            memcpy(mutex, &entry.mutex, sizeof(FileMutex));
            return;
        }
    }

    *mutex = no_mutex;
}

void file_mutex_lock(const FileMutex* mutex) {
    if (mutex->lock) {
        mutex->lock();
    }
}

bool file_mutex_try_lock(const FileMutex* mutex, TickType_t timeout) {
    if (mutex->try_lock) {
        return mutex->try_lock(timeout);
    }
    return true;
}

void file_mutex_unlock(const FileMutex* mutex) {
    if (mutex->unlock) {
        mutex->unlock();
    }
}

}
