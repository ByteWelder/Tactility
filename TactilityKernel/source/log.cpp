// SPDX-License-Identifier: Apache-2.0

#ifndef ESP_PLATFORM

#include <tactility/log.h>
#include <tactility/log_queue.h>

#include <mutex>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/time.h>

namespace {

constexpr auto MINIMUM_LOG_LEVEL = LOG_LEVEL_DEBUG;

const char* get_log_color(LogLevel level) {
    using enum LogLevel;
    switch (level) {
        case LOG_LEVEL_ERROR:
            return "\033[1;31m";
        case LOG_LEVEL_WARNING:
            return "\033[1;33m";
        case LOG_LEVEL_INFO:
            return "\033[32m";
        case LOG_LEVEL_DEBUG:
            return "\033[36m";
        case LOG_LEVEL_VERBOSE:
            return "\033[37m";
        default:
            return "";
    }
}

inline char get_log_prefix(LogLevel level) {
    using enum LogLevel;
    switch (level) {
        case LOG_LEVEL_ERROR:
            return 'E';
        case LOG_LEVEL_WARNING:
            return 'W';
        case LOG_LEVEL_INFO:
            return 'I';
        case LOG_LEVEL_DEBUG:
            return 'D';
        case LOG_LEVEL_VERBOSE:
            return 'V';
        default:
            return '?';
    }
}

uint64_t get_log_timestamp() {
    static uint64_t base = 0U;
    static std::once_flag init_flag;
    std::call_once(init_flag, []() {
      timeval time {};
      gettimeofday(&time, nullptr);
      base = ((uint64_t)time.tv_sec * 1000U) + (time.tv_usec / 1000U);
    });
    timeval time {};
    gettimeofday(&time, nullptr);
    uint64_t now = ((uint64_t)time.tv_sec * 1000U) + (time.tv_usec / 1000U);
    return now - base;
}

}

extern "C" {

void log_generic(enum LogLevel level, const char* tag, const char* format, ...) {
    if (MINIMUM_LOG_LEVEL >= level) {
        char buffer[LOG_QUEUE_MESSAGE_MAX_LENGTH];
        size_t offset = 0;

        int prefix_len = snprintf(buffer, sizeof(buffer), "%s %c (%" PRIu64 ") %s ",
            get_log_color(level), get_log_prefix(level), get_log_timestamp(), tag);
        if (prefix_len > 0) {
            offset = static_cast<size_t>(prefix_len) < sizeof(buffer) ? static_cast<size_t>(prefix_len) : sizeof(buffer) - 1;
        }

        if (offset < sizeof(buffer)) {
            va_list args;
            va_start(args, format);
            int written = vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
            va_end(args);
            if (written > 0) {
                size_t remaining = sizeof(buffer) - offset;
                offset += static_cast<size_t>(written) < remaining ? static_cast<size_t>(written) : remaining - 1;
            }
        }

        if (offset < sizeof(buffer)) {
            int tail_len = snprintf(buffer + offset, sizeof(buffer) - offset, "\033[0m\n");
            if (tail_len > 0) {
                size_t remaining = sizeof(buffer) - offset;
                offset += static_cast<size_t>(tail_len) < remaining ? static_cast<size_t>(tail_len) : remaining - 1;
            }
        }

        log_queue_write(buffer, offset);
    }
}

}

#endif