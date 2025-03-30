#pragma once

namespace tt {

#define SYSTEM_PARTITION_NAME "system"

#ifdef ESP_PLATFORM
#define MOUNT_POINT_SYSTEM "/system"
#else
#define MOUNT_POINT_SYSTEM "system"
#endif

#define DATA_PARTITION_NAME "data"

#ifdef ESP_PLATFORM
#define MOUNT_POINT_DATA "/data"
#else
#define MOUNT_POINT_DATA "data"
#endif

} // namespace
