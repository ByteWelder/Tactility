#pragma once

#include "LoggerAdapter.h"
#include "LoggerAdapterShared.h"

#include <esp_log.h>
#include <sstream>

namespace tt {

inline esp_log_level_t toEspLogLevel(LogLevel level) {
    switch (level) {
        case LogLevel::Error:
            return ESP_LOG_ERROR;
        case LogLevel::Warning:
            return ESP_LOG_WARN;
        case LogLevel::Info:
            return ESP_LOG_INFO;
        case LogLevel::Debug:
            return ESP_LOG_DEBUG;
        case LogLevel::Verbose:
        default:
            return ESP_LOG_VERBOSE;
    }
}

static const LoggerAdapter espLoggerAdapter = [](LogLevel level, const char* tag, const char* message) {
    constexpr auto COLOR_RESET = "\033[0m";
    constexpr auto COLOR_GREY = "\033[37m";
    std::stringstream buffer;
    buffer << COLOR_GREY << esp_log_timestamp() << " [" << toTagColour(level) << toPrefix(level) << COLOR_GREY << "] [" << COLOR_RESET << tag << COLOR_GREY << "] " << toMessageColour(level) << message << COLOR_RESET << std::endl;
    esp_log_write(toEspLogLevel(level), tag, "%s", buffer.str().c_str());
};

}
