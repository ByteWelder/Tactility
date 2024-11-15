#pragma once

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
#define TT_LOG_T(tag, format, ...) \
    ESP_LOGV(tag, format, ##__VA_ARGS__)

#else

typedef enum {
    LogLevelError,
    LogLevelWarning,
    LogLevelInfo,
    LogLevelDebug,
    LogLevelTrace
} LogLevel;

void tt_log(LogLevel level, const char* tag, const char* format, ...);

#define TT_LOG_E(tag, format, ...) \
    tt_log(LogLevelError, tag, format, ##__VA_ARGS__)
#define TT_LOG_W(tag, format, ...) \
    tt_log(LogLevelWarning, tag, format, ##__VA_ARGS__)
#define TT_LOG_I(tag, format, ...) \
    tt_log(LogLevelInfo, tag, format, ##__VA_ARGS__)
#define TT_LOG_D(tag, format, ...) \
    tt_log(LogLevelDebug, tag, format, ##__VA_ARGS__)
#define TT_LOG_T(tag, format, ...) \
    tt_log(LOG_LEVEL_TRACE, tag, format, ##__VA_ARGS__)

#endif // ESP_TARGET
