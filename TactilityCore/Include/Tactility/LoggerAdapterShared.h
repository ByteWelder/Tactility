#pragma once

#include "LoggerCommon.h"

namespace tt {

inline const char* toTagColour(LogLevel level) {
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

inline const char* toMessageColour(LogLevel level) {
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

inline char toPrefix(LogLevel level) {
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
        default:
            return 'V';
    }
}

}