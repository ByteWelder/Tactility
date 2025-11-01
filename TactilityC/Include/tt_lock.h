#pragma once

#include "tt_kernel.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A handle that represents a lock instance. A lock could be a Mutex or similar construct */
typedef void* LockHandle;

typedef enum {
    MutexTypeNormal,
    MutexTypeRecursive
} TtMutexType;

/**
 * Allocate a new mutex instance
 * @param[in] type specify if the mutex is either a normal one, or whether it can recursively (re)lock
 * @return the allocated lock handle
 */
LockHandle tt_lock_alloc_mutex(TtMutexType type);

/**
 * Allocate a lock for a file or folder.
 * Locking is required before reading files for filesystems that are on a shared bus (e.g. SPI SD card sharing the bus with the display)
 * @param path the path to create the lock for
 * @return the allocated lock handle
 */
LockHandle tt_lock_alloc_for_path(const char* path);

/**
 * Attempt to lock the lock.
 * @param[in] handle the handle that represents the mutex instance
 * @param[in] timeout the maximum amount of ticks to wait when trying to lock
 * @return true when the lock was acquired
 */
bool tt_lock_acquire(LockHandle handle, TickType timeout);

/**
 * Attempt to unlock the lock.
 * @param[in] handle the handle that represents the mutex instance
 * @return true when the lock was unlocked
 */
bool tt_lock_release(LockHandle handle);

/** Free the memory for this lock
 * This does not auto-release the lock.
 * @param[in] handle the handle that represents the mutex instance
 */
void tt_lock_free(LockHandle handle);

#ifdef __cplusplus
}
#endif