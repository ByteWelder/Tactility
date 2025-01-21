#pragma once

#ifdef ESP_PLATFORM

#include "esp_err.h"

namespace tt {

esp_err_t initPartitionsEsp();

} // namespace

#endif // ESP_PLATFORM