#pragma once

#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TT_LOG_E(tag, format, ...) \
    ESP_LOGE(tag, format, ##__VA_ARGS__)
#define TT_LOG_W(tag, format, ...) \
    ESP_LOGW(tag, format, ##__VA_ARGS__)
#define TT_LOG_I(tag, format, ...) \
    ESP_LOGI(tag, format, ##__VA_ARGS__)
#define TT_LOG_D(tag, format, ...) \
    ESP_LOGD(tag, format, ##__VA_ARGS__)
#define TT_LOG_T(tag, format, ...) \
    ESP_LOGT(tag, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
