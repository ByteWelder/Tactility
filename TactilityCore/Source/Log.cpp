#ifndef ESP_PLATFORM

#include "Log.h"

#include <cstdint>
#include <sys/time.h>

namespace tt {

static char toPrefix(LogLevel level) {
    switch (level) {
        case LogLevelError:
            return 'E';
        case LogLevelWarning:
            return 'W';
        case LogLevelInfo:
            return 'I';
        case LogLevelDebug:
            return 'D';
        case LogLevelTrace:
            return 'T';
        default:
            return '?';
    }
}

static const char* toColour(LogLevel level) {
    switch (level) {
        case LogLevelError:
            return "\033[1;31m";
        case LogLevelWarning:
            return "\033[33m";
        case LogLevelInfo:
            return "\033[32m";
        case LogLevelDebug:
            return "\033[1;37m";
        case LogLevelTrace:
            return "\033[37m";
        default:
            return "";
    }
}

static uint64_t getTimestamp() {
#ifdef ESP_PLATFORM
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        return clock() / CLOCKS_PER_SEC * 1000;
    }
    static uint32_t base = 0;
    if (base == 0 && xPortGetCoreID() == 0) {
        base = clock() / CLOCKS_PER_SEC * 1000;
    }
    TickType_t tick_count = xPortInIsrContext() ? xTaskGetTickCountFromISR() : xTaskGetTickCount();
    return base + tick_count * (1000 / configTICK_RATE_HZ);
#else
    static uint64_t base = 0;

    struct timeval time {};
    gettimeofday(&time, nullptr);
    uint64_t now = ((uint64_t)time.tv_sec * 1000) + (time.tv_usec / 1000);
    if (base == 0) {
        base = now;
    }
    return now - base;
#endif
}

void log(LogLevel level, const char* tag, const char* format, ...) {
    printf(
        "%s%c (%lu) %s: ",
        toColour(level),
        toPrefix(level),
        getTimestamp(),
        tag
    );

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\033[0m\n");
}

} // namespace

#endif