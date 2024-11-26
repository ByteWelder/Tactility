#pragma once

#ifdef ESP_TARGET

#include "esp_err.h"

namespace tt {

esp_err_t initEspPartitions();

} // namespace

#endif // ESP_TARGET