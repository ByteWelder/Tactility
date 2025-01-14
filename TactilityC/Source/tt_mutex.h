#pragma once

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** A handle that represents a mutex instance */
typedef void* MutexHandle;

enum TtMutexType {
    MUTEX_TYPE_NORMAL,
    MUTEX_TYPE_RECURSIVE
};

/**
 * Allocate a new mutex instance
 * @param[in] type specify if the mutex is either a normal one, or whether it can recursively (re)lock
 * @return the allocated instance
 */
MutexHandle tt_mutex_alloc(enum TtMutexType type);

/** Free up the memory of the specified mutex instance. */
void tt_mutex_free(MutexHandle handle);

/**
 * Attempt to lock a mutex.
 * @param[in] handle the handle that represents the mutex instance
 * @param[in] timeout the maximum amount of ticks to wait when trying to lock
 * @return true when the lock was acquired
 */
bool tt_mutex_lock(MutexHandle handle, TickType_t timeout);

/**
 * Attempt to unlock a mutex.
 * @param[in] handle the handle that represents the mutex instance
 * @param[in] timeout the maximum amount of ticks to wait when trying to unlock
 * @return true when the lock was unlocked
 */
bool tt_mutex_unlock(MutexHandle handle);

#ifdef __cplusplus
}
#endif