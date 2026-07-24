// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <tactility/freertos/freertos.h>

#ifdef __cplusplus
extern "C" {
#endif

struct FileMutex {
    void (*lock)();
    bool (*try_lock)(TickType_t timeout);
    void (*unlock)();
};

void file_register_mutex(const char* path, const FileMutex* mutex);

void file_get_mutex(const char* path, struct FileMutex* mutex);

void file_lock(struct FileMutex* mutex);

bool file_try_lock(struct FileMutex* mutex, TickType_t timeout);

void file_unlock(struct FileMutex* mutex);

#ifdef __cplusplus
}
#endif
