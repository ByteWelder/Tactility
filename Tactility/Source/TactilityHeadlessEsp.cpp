#ifdef ESP_PLATFORM

#include "Tactility/PartitionsEsp.h"
#include "Tactility/TactilityCore.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

namespace tt {

constexpr auto* TAG = "Tactility";

// Initialize NVS
static void initNvs() {
    TT_LOG_I(TAG, "Init NVS");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        TT_LOG_I(TAG, "NVS erasing");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

static void initNetwork() {
    TT_LOG_I(TAG, "Init network");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void initEsp() {
    initNvs();
    initPartitionsEsp();
    initNetwork();
}

} // namespace

#endif
