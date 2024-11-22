#pragma once

#ifdef ESP_TARGET

#include "esp_err.h"

namespace tt {

esp_err_t esp_partitions_init();

} // namespace

#endif // ESP_TARGET