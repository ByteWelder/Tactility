// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/** Fixed size of an app instance's fd table. FD allocation uses the lowest unused index >= 3. */
#define APP_MAX_FDS 16

/**
 * FD-table dispatch for read()/write()/close(). When the calling task belongs to a running app
 * instance and @a fd is one that instance bound/allocated itself, it's looked up in that
 * instance's own fd table. Otherwise (no app instance, e.g. a kernel service task; or an app
 * instance's own fd that was never bound through app-module, e.g. a real file from fopen()/
 * open(), which this layer never intercepts) @a fd is a real underlying fd and the call falls
 * through to the real syscall unchanged.
 * @return number of bytes transferred, 0 on EOF (read) or a closed peer (write), or -1 with
 * errno set on failure. This is the fd-layer translation of AppFileOps::read()/write()'s ssize_t
 * and AppFileOps::close()'s error_t.
 */
ssize_t app_io_read(int fd, void* buffer, size_t size);
ssize_t app_io_write(int fd, const void* buffer, size_t size);
int app_io_close(int fd);

#ifdef __cplusplus
}
#endif
