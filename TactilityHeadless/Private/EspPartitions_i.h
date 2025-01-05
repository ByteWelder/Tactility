#pragma once

#ifdef ESP_TARGET

#include "esp_err.h"

namespace tt {

esp_err_t initPartitionsEsp();

} // namespace

#endif // ESP_TARGET