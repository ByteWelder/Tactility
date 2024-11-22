#ifdef ESP_TARGET

#include "esp_partitions.h"
#include "esp_spiffs.h"
#include "log.h"
#include "nvs_flash.h"

namespace tt {

static const char* TAG = "filesystem";

static esp_err_t nvs_flash_init_safely() {
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        result = nvs_flash_init();
    }
    return result;
}

static esp_err_t spiffs_init(esp_vfs_spiffs_conf_t* conf) {
    esp_err_t ret = esp_vfs_spiffs_register(conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            TT_LOG_E(TAG, "Failed to mount or format filesystem %s", conf->base_path);
        } else if (ret == ESP_ERR_NOT_FOUND) {
            TT_LOG_E(TAG, "Failed to find SPIFFS partition %s", conf->base_path);
        } else {
            TT_LOG_E(TAG, "Failed to initialize SPIFFS %s (%s)", conf->base_path, esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = -1, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        TT_LOG_E(TAG, "Failed to get SPIFFS partition information for %s (%s)", conf->base_path, esp_err_to_name(ret));
    } else {
        TT_LOG_I(TAG, "Partition size for %s: total: %d, used: %d", conf->base_path, total, used);
    }
    return ESP_OK;
}

esp_err_t esp_partitions_init() {
    ESP_ERROR_CHECK(nvs_flash_init_safely());

    esp_vfs_spiffs_conf_t assets_spiffs = {
        .base_path = MOUNT_POINT_ASSETS,
        .partition_label = NULL,
        .max_files = 100,
        .format_if_mount_failed = false
    };

    if (spiffs_init(&assets_spiffs) != ESP_OK) {
        return ESP_FAIL;
    }

    esp_vfs_spiffs_conf_t config_spiffs = {
        .base_path = MOUNT_POINT_CONFIG,
        .partition_label = "config",
        .max_files = 100,
        .format_if_mount_failed = false
    };

    if (spiffs_init(&config_spiffs) != ESP_OK) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

} // namespace

#endif // ESP_TARGET
