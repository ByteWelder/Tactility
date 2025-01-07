#pragma once

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void* SemaphoreHandle;

SemaphoreHandle tt_semaphore_alloc(uint32_t maxCount, TickType_t initialCount);
void tt_semaphore_free(SemaphoreHandle handle);
bool tt_semaphore_acquire(SemaphoreHandle handle, TickType_t timeoutTicks);
bool tt_semaphore_release(SemaphoreHandle handle);
uint32_t tt_semaphore_get_count(SemaphoreHandle handle);

#ifdef __cplusplus
}
#endif