#pragma once

#ifdef ESP_TARGET

#include "esp_err.h"

namespace tt {

#define MOUNT_POINT_ASSETS "/assets"
#define MOUNT_POINT_CONFIG "/config"

esp_err_t esp_partitions_init();

} // namespace

#endif // ESP_TARGET