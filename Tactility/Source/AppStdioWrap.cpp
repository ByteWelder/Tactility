// SPDX-License-Identifier: Apache-2.0

// Paired with -Wl,--wrap=read/write/close - see Tactility/CMakeLists.txt (POSIX) and the
// top-level CMakeLists.txt (ESP32) for where that's applied. On a platform where it isn't
// (currently: macOS, whose linker doesn't support --wrap), these are simply never called - real
// read()/write()/close() calls go straight through unredirected.
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

// region glibc stdio wraps
//
// glibc's printf/fprintf/etc are compiled into libc.so and call an internal, non-exported write()
// alias - --wrap=write (above) can't reach that call, only calls WE make to the public symbol.
// These wraps instead redirect calls WE make to printf/fprintf/etc, the same trick as read/write/
// close above. Newlib (ESP-IDF) doesn't have this gap - its stdio does call the wrappable syscall
// stubs - so Tactility/CMakeLists.txt only applies the matching -Wl,--wrap= flags on POSIX.
//
// Scoped to the printf/getc families only: fread/fwrite take an arbitrary FILE* and are already
// used sitewide for real file I/O (e.g. File.cpp's readBinaryInternal), so wrapping them would
// route every such call through this file's stdin/stdout check - a correctness risk for unrelated
// code that isn't worth taking here. putc/getc are excluded too since glibc defines them as
// macros, not real calls, so wrapping those symbols wouldn't reliably intercept them.

#if !defined(ESP_PLATFORM) && !defined(__APPLE__)

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unistd.h>

extern "C" {
int __real_vfprintf(FILE* stream, const char* format, va_list args);
int __real_fputs(const char* s, FILE* stream);
int __real_fputc(int c, FILE* stream);
int __real_fgetc(FILE* stream);
char* __real_fgets(char* buffer, int size, FILE* stream);
}

namespace {

void writeAllToStdout(const void* data, size_t size) {
    const auto* bytes = static_cast<const char*>(data);
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t written = app_io_write(STDOUT_FILENO, bytes, remaining);
        if (written <= 0) {
            break;
        }
        bytes += written;
        remaining -= static_cast<size_t>(written);
    }
}

// Formats into stdout via app_io_write() rather than through a FILE*'s own buffering, since that
// buffering is exactly what glibc's internal write() call sidesteps --wrap for in the first place.
int formatToStdout(const char* format, va_list args) {
    char stackBuffer[256];
    va_list argsForStack;
    va_copy(argsForStack, args);
    int needed = vsnprintf(stackBuffer, sizeof(stackBuffer), format, argsForStack);
    va_end(argsForStack);
    if (needed < 0) {
        return needed;
    }
    if (static_cast<size_t>(needed) < sizeof(stackBuffer)) {
        writeAllToStdout(stackBuffer, static_cast<size_t>(needed));
        return needed;
    }
    auto heapBuffer = std::make_unique<char[]>(static_cast<size_t>(needed) + 1);
    va_list argsForHeap;
    va_copy(argsForHeap, args);
    vsnprintf(heapBuffer.get(), static_cast<size_t>(needed) + 1, format, argsForHeap);
    va_end(argsForHeap);
    writeAllToStdout(heapBuffer.get(), static_cast<size_t>(needed));
    return needed;
}

int readOneFromStdin(char& out) {
    return static_cast<int>(app_io_read(STDIN_FILENO, &out, 1));
}

} // namespace

extern "C" {

int __wrap_vprintf(const char* format, va_list args) {
    return formatToStdout(format, args);
}

int __wrap_printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = formatToStdout(format, args);
    va_end(args);
    return result;
}

int __wrap_vfprintf(FILE* stream, const char* format, va_list args) {
    if (stream == stdout) {
        return formatToStdout(format, args);
    }
    return __real_vfprintf(stream, format, args);
}

int __wrap_fprintf(FILE* stream, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = (stream == stdout) ? formatToStdout(format, args) : __real_vfprintf(stream, format, args);
    va_end(args);
    return result;
}

int __wrap_puts(const char* s) {
    writeAllToStdout(s, strlen(s));
    writeAllToStdout("\n", 1);
    return 0;
}

int __wrap_fputs(const char* s, FILE* stream) {
    if (stream == stdout) {
        writeAllToStdout(s, strlen(s));
        return 0;
    }
    return __real_fputs(s, stream);
}

int __wrap_putchar(int c) {
    auto ch = static_cast<char>(c);
    writeAllToStdout(&ch, 1);
    return c;
}

int __wrap_fputc(int c, FILE* stream) {
    if (stream == stdout) {
        return __wrap_putchar(c);
    }
    return __real_fputc(c, stream);
}

int __wrap_getchar() {
    char c;
    return readOneFromStdin(c) == 1 ? static_cast<unsigned char>(c) : EOF;
}

int __wrap_fgetc(FILE* stream) {
    if (stream == stdin) {
        return __wrap_getchar();
    }
    return __real_fgetc(stream);
}

char* __wrap_fgets(char* buffer, int size, FILE* stream) {
    if (stream != stdin) {
        return __real_fgets(buffer, size, stream);
    }
    if (size <= 0) {
        return nullptr;
    }
    int i = 0;
    for (; i < size - 1; ++i) {
        char c;
        if (readOneFromStdin(c) != 1) {
            break;
        }
        buffer[i] = c;
        if (c == '\n') {
            ++i;
            break;
        }
    }
    if (i == 0) {
        return nullptr;
    }
    buffer[i] = '\0';
    return buffer;
}

}

#endif // !ESP_PLATFORM && !__APPLE__

// endregion
