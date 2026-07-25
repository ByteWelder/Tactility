// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/freertos/freertos.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set of lock/try_lock/unlock callbacks backing a filesystem mount's mutex.
 * Any field left null is treated as a no-op by file_mutex_lock/try_lock/unlock.
 */
struct FileMutex {
    void (*lock)();
    bool (*try_lock)(uint32_t timeout);
    void (*unlock)();
};

/**
 * @brief Registers a mutex for a mount path (e.g. "/sdcard") and its descendants.
 * @param[in] mutex callbacks to associate with the path; a copy is stored
 * @param[in] path mount path this mutex serializes access to
 * @note No-op if a mutex is already registered for this exact path.
 */
void file_mutex_register(const struct FileMutex* mutex, const char* path);

/**
 * @brief Looks up the mutex registered for path or one of its ancestor mount paths.
 * @param[out] mutex receives the matching mutex, or an all-null (no-op) mutex if none matches
 * @param[in] path file or directory path to look up
 */
void file_mutex_get(struct FileMutex* mutex, const char* path);

/** @brief Locks mutex. No-op if mutex->lock is null. */
void file_mutex_lock(const struct FileMutex* mutex);

/**
 * @brief Attempts to lock mutex within timeout.
 * @return true if locked (or mutex->try_lock is null), false on timeout
 */
bool file_mutex_try_lock(const struct FileMutex* mutex, TickType_t timeout);

/** @brief Unlocks mutex. No-op if mutex->unlock is null. */
void file_mutex_unlock(const struct FileMutex* mutex);

#ifdef __cplusplus
}
#endif
