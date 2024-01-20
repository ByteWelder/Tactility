#pragma once

#include "esp_err.h"

#define MOUNT_POINT_ASSETS "/assets"
#define MOUNT_POINT_CONFIG "/config"

esp_err_t tt_partitions_init();

