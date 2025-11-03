#pragma once

#ifdef ESP_PLATFORM

#include "esp_err.h"
#include "esp_vfs_fat.h"

namespace tt {

esp_err_t initPartitionsEsp();
wl_handle_t getDataPartitionWlHandle();

} // namespace

#endif // ESP_PLATFORM
