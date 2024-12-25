#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef void* SemaphoreHandle;

SemaphoreHandle tt_semaphore_alloc(uint32_t maxCount, uint32_t initialCount);
void tt_semaphore_free(SemaphoreHandle handle);
bool tt_semaphore_acquire(SemaphoreHandle handle, uint32_t timeoutTicks);
bool tt_semaphore_release(SemaphoreHandle handle);
uint32_t tt_semaphore_get_count(SemaphoreHandle handle);

#ifdef __cplusplus
}
#endif