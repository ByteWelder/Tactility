#pragma once

#ifdef ESP_PLATFORM

#include <esp_log.h>
#include "Tactility/LogCommon.h"

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

#endif // ESP_PLATFORM
