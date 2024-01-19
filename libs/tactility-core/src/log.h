#pragma once

#ifdef ESP_PLATFORM
#include "esp_log.h"
#else
#include <stdarg.h>
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ESP_PLATFORM

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
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE
} LogLevel;

void tt_log(LogLevel level, const char* tag, const char* format, ...);

#define TT_LOG_E(tag, format, ...) \
    tt_log(LOG_LEVEL_ERROR, tag, format, ##__VA_ARGS__)
#define TT_LOG_W(tag, format, ...) \
    tt_log(LOG_LEVEL_WARNING, tag, format, ##__VA_ARGS__)
#define TT_LOG_I(tag, format, ...) \
    tt_log(LOG_LEVEL_INFO, tag, format, ##__VA_ARGS__)
#define TT_LOG_D(tag, format, ...) \
    tt_log(LOG_LEVEL_DEBUG, tag, format, ##__VA_ARGS__)
#define TT_LOG_T(tag, format, ...) \
    tt_log(LOG_LEVEL_TRACE, tag, format, ##__VA_ARGS__)

#endif

#ifdef __cplusplus
}
#endif
