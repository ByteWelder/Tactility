// SPDX-License-Identifier: Apache-2.0
#ifdef ESP_PLATFORM

#include <app/io.h>

#include <sys/types.h>

extern "C" {

ssize_t __wrap_read(int fd, void* buffer, size_t size) {
    return app_io_read(fd, buffer, size);
}

ssize_t __wrap_write(int fd, const void* buffer, size_t size) {
    return app_io_write(fd, buffer, size);
}

int __wrap_close(int fd) {
    return app_io_close(fd);
}

}

#endif // ESP_PLATFORM
