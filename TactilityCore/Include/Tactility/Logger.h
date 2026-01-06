#pragma once

#include "LoggerAdapter.h"
#include "LoggerSettings.h"

#ifdef ESP_PLATFORM
#include "LoggerAdapterEsp.h"
#else
#include "LoggerAdapterGeneric.h"
#endif

#include <format>

namespace tt {

#ifdef ESP_PLATFORM
static LoggerAdapter defaultLoggerAdapter = espLoggerAdapter;
#else
static LoggerAdapter defaultLoggerAdapter = genericLoggerAdapter;
#endif

class Logger {

    const char* tag;

public:

    explicit Logger(const char* tag) : tag(tag) {}

    template <typename... Args>
    void log(LogLevel level, std::format_string<Args...> format, Args&&... args) const {
        std::string message = std::format(format, std::forward<Args>(args)...);
        defaultLoggerAdapter(level, tag, message.c_str());
    }

    template <typename... Args>
    void verbose(std::format_string<Args...> format, Args&&... args) const {
        log(LogLevel::Verbose, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(std::format_string<Args...> format, Args&&... args) const {
        log(LogLevel::Debug, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::format_string<Args...> format, Args&&... args) const {
        log(LogLevel::Info, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::format_string<Args...> format, Args&&... args) const {
        log(LogLevel::Warning, format, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::format_string<Args...> format, Args&&... args) const {
        log(LogLevel::Error, format, std::forward<Args>(args)...);
    }

    bool isLoggingVerbose() const { return LogLevel::Verbose <= LOG_LEVEL; }

    bool isLoggingDebug() const { return LogLevel::Debug <= LOG_LEVEL; }

    bool isLoggingInfo() const { return LogLevel::Info <= LOG_LEVEL; }

    bool isLoggingWarning() const { return LogLevel::Warning <= LOG_LEVEL; }

    bool isLoggingError() const { return LogLevel::Error <= LOG_LEVEL; }
};

}
