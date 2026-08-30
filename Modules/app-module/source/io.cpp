// SPDX-License-Identifier: Apache-2.0
#include <app/io.h>

#include <app/private/fd_table.h>
#include <app/private/ledger.h>
#include <app/scheduler.h>

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
    // No app instance, or an app instance that never bound/allocated this fd itself (e.g. a real
    // file fd from fopen()/open(), which app-module never intercepts; see app/io.h). Either way
    // @a fd is a real underlying fd, not one of ours.
    if (table == nullptr || !app_fd_table_get_and_retain(table, fd, &file)) {
#ifdef ESP_PLATFORM
        return __real_read(fd, buffer, size);
#else
        return ::read(fd, buffer, size);
#endif
    }
    ssize_t result = file.ops->read(file.object, buffer, size);
    if (file.ops->release != nullptr) {
        file.ops->release(file.object);
    }
    return result;
}

ssize_t app_io_write(int fd, const void* buffer, size_t size) {
    AppFdTable* table = current_app_fd_table();
    AppFile file {};
    if (table == nullptr || !app_fd_table_get_and_retain(table, fd, &file)) {
#ifdef ESP_PLATFORM
        return __real_write(fd, buffer, size);
#else
        return ::write(fd, buffer, size);
#endif
    }
    ssize_t result = file.ops->write(file.object, buffer, size);
    if (file.ops->release != nullptr) {
        file.ops->release(file.object);
    }
    return result;
}

int app_io_close(int fd) {
    AppFdTable* table = current_app_fd_table();
    if (table == nullptr || app_fd_table_close(table, fd) != ERROR_NONE) {
#ifdef ESP_PLATFORM
        return __real_close(fd);
#else
        return ::close(fd);
#endif
    }
    return 0;
}

} // extern "C"
