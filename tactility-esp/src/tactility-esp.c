#include "tactility.h"

#include "esp_event.h"
#include "esp_lvgl_port.h"
#include "esp_netif.h"
#include "graphics_i.h"
#include "hardware_i.h"
#include "nvs_flash.h"
#include "partitions.h"
#include "services/loader/loader.h"
#include "services/wifi/wifi_credentials.h"
#include "ui/lvgl_sync.h"

#define TAG "tactility"

static bool lvgl_lock_impl(int timeout_ticks) {
    return lvgl_port_lock(timeout_ticks);
}

static void lvgl_unlock_impl() {
    lvgl_port_unlock();
}

void tt_esp_init(const HardwareConfig* hardware_config) {
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

    tt_lvgl_sync_set(&lvgl_lock_impl, &lvgl_unlock_impl);

    Hardware hardware = tt_hardware_init(hardware_config);
    /*Lvgl lvgl =*/tt_graphics_init(&hardware);
}
