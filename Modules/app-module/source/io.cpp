// SPDX-License-Identifier: Apache-2.0
#include <app/io.h>

#include <app/private/fd_table.h>
#include <app/private/ledger.h>
#include <app/scheduler.h>

#include <cerrno>

#ifdef ESP_PLATFORM
extern "C" {
ssize_t __real_read(int fd, void* buffer, size_t size);
ssize_t __real_write(int fd, const void* buffer, size_t size);
int __real_close(int fd);
}
#else
#include <unistd.h>
#endif

namespace {

// NULL for a caller not running as an app instance (e.g. a kernel service task). @a fd is then
// a real underlying fd, handled by the caller falling through to the real syscall.
AppFdTable* current_app_fd_table() {
    AppInstanceId app_id = app_scheduler_current_app_id();
    if (app_id == 0) {
        return nullptr;
    }
    auto& ledger = app_ledger();
    mutex_lock(&ledger.mutex);
    auto iterator = ledger.instances.find(app_id);
    AppFdTable* table = (iterator != ledger.instances.end()) ? &iterator->second.fd_table : nullptr;
    mutex_unlock(&ledger.mutex);
    return table;
}

} // namespace

extern "C" {

ssize_t app_io_read(int fd, void* buffer, size_t size) {
    AppFdTable* table = current_app_fd_table();
    AppFile file {};
    if (table != nullptr && app_fd_table_get_and_retain(table, fd, &file)) {
        ssize_t result = file.ops->read(file.object, buffer, size);
        if (file.ops->release != nullptr) {
            file.ops->release(file.object);
        }
        return result;
    }
    // @a fd isn't currently bound. If this table has never touched it either, it's a real
    // underlying fd (e.g. from fopen()/open(), which app-module never intercepts; see app/io.h),
    // so fall through. Otherwise it's one of ours that's already been closed: a real EBADF, not
    // a real fd to hand to the platform (which could belong to something else entirely by now).
    if (table != nullptr && app_fd_table_is_app_owned(table, fd)) {
        errno = EBADF;
        return -1;
    }
#ifdef ESP_PLATFORM
    return __real_read(fd, buffer, size);
#else
    return ::read(fd, buffer, size);
#endif
}

ssize_t app_io_write(int fd, const void* buffer, size_t size) {
    AppFdTable* table = current_app_fd_table();
    AppFile file {};
    if (table != nullptr && app_fd_table_get_and_retain(table, fd, &file)) {
        ssize_t result = file.ops->write(file.object, buffer, size);
        if (file.ops->release != nullptr) {
            file.ops->release(file.object);
        }
        return result;
    }
    if (table != nullptr && app_fd_table_is_app_owned(table, fd)) {
        errno = EBADF;
        return -1;
    }
#ifdef ESP_PLATFORM
    return __real_write(fd, buffer, size);
#else
    return ::write(fd, buffer, size);
#endif
}

int app_io_close(int fd) {
    AppFdTable* table = current_app_fd_table();
    if (table != nullptr) {
        error_t result = app_fd_table_close(table, fd);
        if (result == ERROR_NONE) {
            return 0;
        }
        if (app_fd_table_is_app_owned(table, fd)) {
            errno = EBADF;
            return -1;
        }
    }
#ifdef ESP_PLATFORM
    return __real_close(fd);
#else
    return ::close(fd);
#endif
}

} // extern "C"
