#ifdef ESP_PLATFORM

#include "Tactility/service/espnow/EspNow.h"
#include "Tactility/service/wifi/Wifi.h"
#include <Tactility/Log.h>

#include <esp_now.h>
#include <esp_wifi.h>

namespace tt::service::espnow {

constexpr const char* TAG = "EspNowService";

static bool disableWifiService() {
    auto wifi_state = wifi::getRadioState();
    if (wifi_state != wifi::RadioState::Off && wifi_state != wifi::RadioState::OffPending) {
        wifi::setEnabled(false);
    }

    if (wifi::getRadioState() == wifi::RadioState::Off) {
        return true;
    } else {
        TickType_t timeout_time = kernel::getTicks() + kernel::millisToTicks(2000);
        while (kernel::getTicks() < timeout_time && wifi::getRadioState() != wifi::RadioState::Off) {
            kernel::delayTicks(50);
        }

        return wifi::getRadioState() == wifi::RadioState::Off;
    }
}

bool initWifi(const EspNowConfig& config) {
    if (!disableWifiService()) {
        TT_LOG_E(TAG, "Failed to disable wifi");
        return false;
    }

    wifi_mode_t mode;
    if (config.mode == Mode::Station) {
        mode = wifi_mode_t::WIFI_MODE_STA;
    } else {
        mode = wifi_mode_t::WIFI_MODE_AP;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        TT_LOG_E(TAG, "esp_wifi_init() failed");
        return false;
    }

    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
        TT_LOG_E(TAG, "esp_wifi_set_storage() failed");
        return false;
    }

    if (esp_wifi_set_mode(mode) != ESP_OK) {
        TT_LOG_E(TAG, "esp_wifi_set_mode() failed");
        return false;
    }

    if (esp_wifi_start() != ESP_OK) {
        TT_LOG_E(TAG, "esp_wifi_start() failed");
        return false;
    }

    if (esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
        TT_LOG_E(TAG, "esp_wifi_set_channel() failed");
        return false;
    }

    if (config.longRange) {
        wifi_interface_t wifi_interface;
        if (config.mode == Mode::Station) {
            wifi_interface = WIFI_IF_STA;
        } else {
            wifi_interface = WIFI_IF_AP;
        }

        if (esp_wifi_set_protocol(wifi_interface, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR) != ESP_OK) {
            TT_LOG_W(TAG, "esp_wifi_set_protocol() for long range failed");
        }
    }

    return true;
}

bool deinitWifi() {
    if (esp_wifi_stop() != ESP_OK) {
        TT_LOG_E(TAG, "Failed to stop radio");
        return false;
    }

    if (esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to unset mode");
    }

    if (esp_wifi_deinit() != ESP_OK) {
        TT_LOG_E(TAG, "Failed to deinit");
    }

    return true;
}

} // namespace tt::service::espnow

#endif // ESP_PLATFORM
