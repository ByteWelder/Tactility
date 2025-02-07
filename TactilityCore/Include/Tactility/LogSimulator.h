#pragma once

#ifndef ESP_PLATFORM

#include "LogCommon.h"

#include <cstdarg>
#include <cstdio>

namespace tt {

void log(LogLevel level, const char* tag, const char* format, ...);

} // namespace

#define TT_LOG_E(tag, format, ...) \
    tt::log(tt::LogLevel::Error, tag, format, ##__VA_ARGS__)
#define TT_LOG_W(tag, format, ...) \
    tt::log(tt::LogLevel::Warning, tag, format, ##__VA_ARGS__)
#define TT_LOG_I(tag, format, ...) \
    tt::log(tt::LogLevel::Info, tag, format, ##__VA_ARGS__)
#define TT_LOG_D(tag, format, ...) \
    tt::log(tt::LogLevel::Debug, tag, format, ##__VA_ARGS__)
#define TT_LOG_V(tag, format, ...) \
    tt::log(tt::LogLevel::Verbose, tag, format, ##__VA_ARGS__)

#endif // ESP_PLATFORM
