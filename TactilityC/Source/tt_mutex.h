#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void* MutexHandle;

enum TtMutexType {
    MUTEX_TYPE_NORMAL,
    MUTEX_TYPE_RECURSIVE
};

MutexHandle tt_mutex_alloc(enum TtMutexType);
void tt_mutex_free(MutexHandle handle);
bool tt_mutex_lock(MutexHandle handle, uint32_t timeoutTicks);
bool tt_mutex_unlock(MutexHandle handle);

#ifdef __cplusplus
}
#endif