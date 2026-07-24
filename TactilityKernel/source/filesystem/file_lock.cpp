// SPDX-License-Identifier: Apache-2.0
#include <tactility/filesystem/file_lock.h>

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
void file_register_mutex(const char* path, const FileMutex* mutex) {
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

void file_get_mutex(const char* path, FileMutex* mutex) {
    for (auto& entry : mutex_entries) {
        if (entry.path.rfind(path) == 0) {
            memcpy(mutex, &entry.mutex, sizeof(FileMutexEntry));
            return;
        }
    }

    memcpy(mutex, &no_mutex, sizeof(FileMutex));
}

void file_lock(FileMutex* mutex) {
    if (mutex->lock) {
        mutex->lock();
    }
}

bool file_try_lock(FileMutex* mutex, TickType_t timeout) {
    if (mutex->try_lock) {
        return mutex->try_lock(timeout);
    }
    return true;
}

void file_unlock(FileMutex* mutex) {
    if (mutex->unlock) {
        mutex->unlock();
    }
}

}
