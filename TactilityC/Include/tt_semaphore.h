#pragma once

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** A handle that represents a semaphore instance */
typedef void* SemaphoreHandle;

/**
 * Allocate a new semaphore instance.
 * @param[in] maxCount the maximum counter value
 * @param[in] initialCount the initial counter value
 * @return the handle that represents the new instance
 */
SemaphoreHandle tt_semaphore_alloc(uint32_t maxCount, TickType_t initialCount);

/** Free up the memory of a specified semaphore instance */
void tt_semaphore_free(SemaphoreHandle handle);

/**
 * Attempt to acquire a semaphore (increase counter)
 * @param[in] handle the instance handle
 * @param[in] timeout the maximum amount of ticks to wait while trying to acquire
 * @return true on successfully acquiring the semaphore (counter is increased)
 */
bool tt_semaphore_acquire(SemaphoreHandle handle, TickType_t timeout);

/**
 * Release an acquired semaphore (decrease counter)
 * @param[in] handle the instance handle
 * @return true on successfully releasing the semaphore (counter is decreased)
 */
bool tt_semaphore_release(SemaphoreHandle handle);

/**
 * Get the counter value of this semaphore instance
 * @param[in] handle the instance handle
 * @return the current counter value (acquisition count)
 */
uint32_t tt_semaphore_get_count(SemaphoreHandle handle);

#ifdef __cplusplus
}
#endif