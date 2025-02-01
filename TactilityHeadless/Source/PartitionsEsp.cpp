#ifdef ESP_PLATFORM

#include "Tactility/PartitionsEsp.h"

#include <Tactility/Log.h>

#include <esp_vfs_fat.h>
#include <nvs_flash.h>

namespace tt {

static const char* TAG = "partitions";

static esp_err_t initNvsFlashSafely() {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result;
}

static wl_handle_t data_wl_handle = WL_INVALID_HANDLE;

esp_err_t initPartitionsEsp() {
    ESP_ERROR_CHECK(initNvsFlashSafely());

    const esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .disk_status_check_enable = false,
        .use_one_fat = true,
    };

    auto system_result = esp_vfs_fat_spiflash_mount_ro("/system", "system", &mount_config);
    if (system_result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to mount /system (%s)", esp_err_to_name(system_result));
    } else {
        TT_LOG_I(TAG, "Mounted /system");
    }

    auto data_result = esp_vfs_fat_spiflash_mount_rw_wl("/data", "data", &mount_config, &data_wl_handle);
    if (data_result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to mount /data (%s)", esp_err_to_name(data_result));
    } else {
        TT_LOG_I(TAG, "Mounted /data");
    }

    return system_result == ESP_OK && data_result == ESP_OK;
}

} // namespace

#endif // ESP_PLATFORM
