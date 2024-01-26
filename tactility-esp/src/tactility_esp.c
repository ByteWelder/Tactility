#include "tactility_core.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "partitions.h"
#include "services/wifi/wifi_credentials.h"

#define TAG "tactility"
void tt_esp_init() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        TT_LOG_I(TAG, "nvs erasing");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    TT_LOG_I(TAG, "nvs initialized");

    // Network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    tt_partitions_init();

    tt_wifi_credentials_init();
}
