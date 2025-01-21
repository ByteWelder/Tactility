#pragma once

#include "LogMessages.h"

#ifdef ESP_PLATFORM
#include <esp_log.h>
#else
#include <cstdarg>
#include <cstdio>
#endif

#if not defined(ESP_PLATFORM) or (defined(CONFIG_SPIRAM_USE_MALLOC) && CONFIG_SPIRAM_USE_MALLOC  == 1)
#define TT_LOG_ENTRY_COUNT 200
#define TT_LOG_MESSAGE_SIZE 128
#else
#define TT_LOG_ENTRY_COUNT 50
#define TT_LOG_MESSAGE_SIZE 50
#endif

namespace tt {

/** Used for log output filtering */
enum class LogLevel {
    None,       /*!< No log output */
    Error,      /*!< Critical errors, software module can not recover on its own */
    Warning,    /*!< Error conditions from which recovery measures have been taken */
    Info,       /*!< Information messages which describe normal flow of events */
    Debug,      /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */
    Verbose     /*!< Bigger chunks of debugging information, or frequent messages which can potentially flood the output. */
};

struct LogEntry {
    LogLevel level = LogLevel::None;
    char message[TT_LOG_MESSAGE_SIZE] = { 0 };
};

/** Make a copy of the currently stored entries.
 * The array size is TT_LOG_ENTRY_COUNT
 * @param[out] outIndex the write index for the next log entry.
 */
LogEntry* copyLogEntries(unsigned int& outIndex);

} // namespace tt


#ifdef ESP_PLATFORM

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
    tt::log(tt::LogLevel::Error, tag, format, ##__VA_ARGS__)
#define TT_LOG_W(tag, format, ...) \
    tt::log(tt::LogLevel::Warning, tag, format, ##__VA_ARGS__)
#define TT_LOG_I(tag, format, ...) \
    tt::log(tt::LogLevel::Info, tag, format, ##__VA_ARGS__)
#define TT_LOG_D(tag, format, ...) \
    tt::log(tt::LogLevel::Debug, tag, format, ##__VA_ARGS__)
#define TT_LOG_V(tag, format, ...) \
    tt::log(tt::LogLevel::Trace, tag, format, ##__VA_ARGS__)

#endif // ESP_PLATFORM
