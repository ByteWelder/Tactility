// SPDX-License-Identifier: Apache-2.0
#include <tactility/filesystem/file_mutex.h>
#include <tactility/concurrent/mutex.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

static const FileMutex no_mutex = {
    .lock = nullptr,
    .try_lock = nullptr,
    .unlock = nullptr,
};

struct FileMutexEntry {
    FileMutexId id;
    std::string path;
    FileMutex mutex;
};

// Guards mutex_entries against concurrent add/get/remove; unrelated to whether a FileMutex's own
// lock/unlock is currently held (file_mutex_get() hands out a copy that stays valid regardless of
// later registry changes - see file_mutex_remove()).
struct FileMutexLedger {
    std::vector<FileMutexEntry> entries;
    FileMutexId next_id = 1;
    Mutex mutex {};

    FileMutexLedger() { mutex_construct(&mutex); }
    ~FileMutexLedger() { mutex_destruct(&mutex); }

    void lock() { mutex_lock(&mutex); }
    void unlock() { mutex_unlock(&mutex); }
};

static FileMutexLedger& get_ledger() {
    static FileMutexLedger ledger;
    return ledger;
}

extern "C" {

FileMutexId file_mutex_add(const FileMutex* mutex, const char* path) {
    auto& ledger = get_ledger();
    ledger.lock();

    for (auto& entry : ledger.entries) {
        if (entry.path == path) {
            FileMutexId existing_id = entry.id;
            ledger.unlock();
            return existing_id;
        }
    }

    FileMutexId new_id = ledger.next_id++;
    ledger.entries.push_back({
        .id = new_id,
        .path = path,
        .mutex = *mutex
    });

    ledger.unlock();
    return new_id;
}

void file_mutex_remove(FileMutexId id) {
    auto& ledger = get_ledger();
    ledger.lock();

    const auto iterator = std::ranges::find_if(ledger.entries, [id](const FileMutexEntry& entry) {
        return entry.id == id;
    });
    if (iterator != ledger.entries.end()) {
        // Plain erase, not swap-and-pop: file_mutex_get() matches first-registered-wins, so
        // removal must preserve the relative order of the remaining entries.
        ledger.entries.erase(iterator);
    }

    ledger.unlock();
}

void file_mutex_get(FileMutex* mutex, const char* path) {
    auto& ledger = get_ledger();
    std::string path_string = path;

    ledger.lock();
    for (auto& entry : ledger.entries) {
        // Match the mount path itself, or a descendant (e.g. "/sdcard" registered, "/sdcard/config.json" requested).
        bool is_match = path_string == entry.path ||
            (entry.path == "/" && !path_string.empty() && path_string[0] == '/') ||
            (path_string.rfind(entry.path, 0) == 0 && path_string[entry.path.size()] == '/');
        if (is_match) {
            memcpy(mutex, &entry.mutex, sizeof(FileMutex));
            ledger.unlock();
            return;
        }
    }
    ledger.unlock();

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
