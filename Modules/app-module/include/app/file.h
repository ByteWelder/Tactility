// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#include <tactility/error.h>
#include <tactility/freertos/freertos.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Which readiness condition to block for in AppFileOps::await(). */
typedef enum {
    APP_FILE_WAIT_READABLE,
    APP_FILE_WAIT_WRITABLE,
} AppFileWait;

/** Bits returned by AppFileOps::poll(). */
#define APP_FILE_READABLE (1u << 0)
#define APP_FILE_WRITABLE (1u << 1)

/**
 * Operations table for one kind of file-like object (stream, file, device, ...). Every function
 * takes the type-erased `object` a concrete AppFile instance was constructed with.
 */
struct AppFileOps {
    ssize_t (*read)(void* object, void* buffer, size_t size);
    ssize_t (*write)(void* object, const void* buffer, size_t size);
    error_t (*close)(void* object);
    error_t (*await)(void* object, AppFileWait wait, TickType_t timeout);
    /** @return bitmask of APP_FILE_READABLE / APP_FILE_WRITABLE. */
    uint32_t (*poll)(void* object);
};

/** A file-descriptor-table entry: an operations table paired with the object it operates on. */
struct AppFile {
    const struct AppFileOps* ops;
    void* object;
};

#ifdef __cplusplus
}
#endif
