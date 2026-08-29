// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/file.h>
#include <app/io.h>

#include <stdbool.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/error.h>

/**
 * Every app instance's fd table (see AppInstanceRecord). `slots` is the backing storage for the
 * AppFile each in-use `fds[]` entry points to: `fds[i]` is either NULL or `&slots[i]`. `mutex`
 * guards `fds`/`slots` only; it is never held while calling into an AppFile's ops (dispatch
 * copies the AppFile out under the lock first), since those calls belong to independently
 * synchronized objects (AppStream's own mutex/event group) and may run concurrently with a
 * close()/bind() replacing an unrelated slot.
 */
struct AppFdTable {
    struct AppFile slots[APP_MAX_FDS];
    struct AppFile* fds[APP_MAX_FDS];
    struct Mutex mutex;
};

#ifdef __cplusplus
extern "C" {
#endif

/** Initializes every fd to the null device (see app/private/null_device.h) and constructs `mutex`. */
void app_fd_table_construct(struct AppFdTable* table);

/** Closes every still-open fd (via AppFileOps::close()) and destructs `mutex`. */
void app_fd_table_teardown(struct AppFdTable* table);

/**
 * Installs {ops, object} at @a fd, closing whatever was previously there first (the null device
 * counts as "previously there" for fds 0-2, so this doubles as their initial stdio binding).
 * @retval ERROR_OUT_OF_RANGE @a fd is outside [0, APP_MAX_FDS)
 */
error_t app_fd_table_bind(struct AppFdTable* table, int fd, const struct AppFileOps* ops, void* object);

/**
 * Installs {ops, object} at the lowest unused fd >= 3.
 * @retval ERROR_RESOURCE the table is full
 */
error_t app_fd_table_allocate(struct AppFdTable* table, const struct AppFileOps* ops, void* object, int* out_fd);

/** @return true and fills @a out_file if @a fd is currently in use, false otherwise. */
bool app_fd_table_get(struct AppFdTable* table, int fd, struct AppFile* out_file);

/**
 * Closes @a fd (via AppFileOps::close()) and frees the slot for reuse.
 * @retval ERROR_NOT_FOUND @a fd is out of range or not currently in use
 */
error_t app_fd_table_close(struct AppFdTable* table, int fd);

#ifdef __cplusplus
}
#endif
