#pragma once

#include "LogMessages.h"

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

typedef enum {
    LogLevelError,
    LogLevelWarning,
    LogLevelInfo,
    LogLevelDebug,
    LogLevelTrace
} LogLevel;

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
