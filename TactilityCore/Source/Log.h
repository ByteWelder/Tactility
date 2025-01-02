#pragma once

#include "LogMessages.h"

#if CONFIG_SPIRAM_USE_MALLOC == 1 or not defined(ESP_PLATFORM)
#define TT_LOG_ENTRY_COUNT 200
#define TT_LOG_MESSAGE_SIZE 128
#else
#define TT_LOG_ENTRY_COUNT 50
#define TT_LOG_MESSAGE_SIZE 50
#endif

namespace tt {

enum LogLevel {
    LogLevelNone,       /*!< No log output */
    LogLevelError,      /*!< Critical errors, software module can not recover on its own */
    LogLevelWarning,    /*!< Error conditions from which recovery measures have been taken */
    LogLevelInfo,       /*!< Information messages which describe normal flow of events */
    LogLevelDebug,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    LogLevelVerbose     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
};

struct LogEntry {
    LogLevel level = LogLevelNone;
    char message[TT_LOG_MESSAGE_SIZE] = { 0 };
};

LogEntry* copyLogEntries(unsigned int& outIndex);

} // namespace tt

#ifdef ESP_TARGET
#include "esp_log.h"
#else
#include <cstdarg>
#include <cstdio>
#endif

#ifdef ESP_TARGET

#define TT_LOG_E(tag, format, ...) \
    ESP_LOGE(tag, format, ##__VA_ARGS__)
#define TT_LOG_W(tag, format, ...) \
    ESP_LOGW(tag, format, ##__VA_ARGS__)
#define TT_LOG_I(tag, format, ...) \
    ESP_LOGI(tag, format, ##__VA_ARGS__)
#define TT_LOG_D(tag, format, ...) \
    ESP_LOGD(tag, format, ##__VA_ARGS__)
#define TT_LOG_V(tag, format, ...) \
    ESP_LOGV(tag, format, ##__VA_ARGS__)

#else

namespace tt {

void log(LogLevel level, const char* tag, const char* format, ...);

} // namespace

#define TT_LOG_E(tag, format, ...) \
    tt::log(tt::LogLevelError, tag, format, ##__VA_ARGS__)
#define TT_LOG_W(tag, format, ...) \
    tt::log(tt::LogLevelWarning, tag, format, ##__VA_ARGS__)
#define TT_LOG_I(tag, format, ...) \
    tt::log(tt::LogLevelInfo, tag, format, ##__VA_ARGS__)
#define TT_LOG_D(tag, format, ...) \
    tt::log(tt::LogLevelDebug, tag, format, ##__VA_ARGS__)
#define TT_LOG_T(tag, format, ...) \
    tt::log(tt::LogLevelTrace, tag, format, ##__VA_ARGS__)

#endif // ESP_TARGET
