#ifndef ESP_PLATFORM

#include "Tactility/Log.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <sys/time.h>

namespace tt {

static char toPrefix(LogLevel level) {
    using enum LogLevel;
    switch (level) {
        case Error:
            return 'E';
        case Warning:
            return 'W';
        case Info:
            return 'I';
        case Debug:
            return 'D';
        case Verbose:
            return 'V';
        default:
            return ' ';
    }
}

static const char* toTagColour(LogLevel level) {
    using enum LogLevel;
    switch (level) {
        case Error:
            return "\033[1;31m";
        case Warning:
            return "\033[1;33m";
        case Info:
            return "\033[32m";
        case Debug:
            return "\033[36m";
        case Verbose:
            return "\033[37m";
        default:
            return "";
    }
}

static const char* toMessageColour(LogLevel level) {
    using enum LogLevel;
    switch (level) {
        case Error:
            return "\033[1;31m";
        case Warning:
            return "\033[1;33m";
        case Info:
        case Debug:
        case Verbose:
            return "\033[0m";
        default:
            return "";
    }
}
static uint64_t getLogTimestamp() {
    static uint64_t base = 0U;
    struct timeval time {};
    gettimeofday(&time, nullptr);
    uint64_t now = ((uint64_t)time.tv_sec * 1000U) + (time.tv_usec / 1000U);
    if (base == 0U) {
        base = now;
    }
    return now - base;
}

void log(LogLevel level, const char* tag, const char* format, ...) {
    std::stringstream buffer;
    buffer << getLogTimestamp() << " [" << toTagColour(level) << toPrefix(level) << "\033[0m" << "] [" << tag << "] " << toMessageColour(level) << format  << "\033[0m\n";

    va_list args;
    va_start(args, format);
    vprintf(buffer.str().c_str(), args);
    va_end(args);
}

} // namespace tt

#endif