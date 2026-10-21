// SPDX-License-Identifier: Apache-2.0
// app-module's io.cpp falls back to __real_read/__real_write/__real_close for kernel-service
// callers. On desktop POSIX those come from app-posix-module's stdio overrides; the web build
// excludes that module (no dlopen) but keeps io.cpp's wrapped-stdio path, so it gets plain
// passthroughs here instead.
#include <unistd.h>

extern "C" {

ssize_t __real_read(int fd, void* buffer, size_t size) {
    return ::read(fd, buffer, size);
}

ssize_t __real_write(int fd, const void* buffer, size_t size) {
    return ::write(fd, buffer, size);
}

int __real_close(int fd) {
    return ::close(fd);
}

}
