#pragma once

namespace tt {

constexpr const char* SYSTEM_PARTITION_NAME = "system";

#ifdef ESP_PLATFORM
constexpr const char* MOUNT_POINT_SYSTEM = "/system";
#else
constexpr const char* MOUNT_POINT_SYSTEM = "system";
#endif

constexpr const char* DATA_PARTITION_NAME = "data";

#ifdef ESP_PLATFORM
constexpr const char* MOUNT_POINT_DATA = "/data";
#else
constexpr const char* MOUNT_POINT_DATA = "data";
#endif

} // namespace
