#pragma once

#include "tt_kernel.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** A handle that represents a lock instance. A lock could be a Mutex or similar construct */
typedef void* LockHandle;

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
 * @param[in] handle the handle that represents the mutex instance
 */
void tt_lock_free(LockHandle handle);

#ifdef __cplusplus
}
#endif