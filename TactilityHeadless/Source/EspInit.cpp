#include "TactilityCore.h"

#ifdef ESP_TARGET

#include "EspPartitions_i.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

namespace tt {

#define TAG "tactility"

// Initialize NVS
static void initEspNvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        TT_LOG_I(TAG, "nvs erasing");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    TT_LOG_I(TAG, "nvs initialized");
}

static void initEspNetwork() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void initEsp() {
    initEspNvs();
    initEspPartitions();
    initEspNetwork();
}

} // namespace

#endif
