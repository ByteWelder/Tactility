#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if defined(CONFIG_TT_WIFI_ENABLED) && !defined(CONFIG_ESP_WIFI_REMOTE_ENABLED)

#include <Tactility/kernel/Kernel.h>
#include <Tactility/Logger.h>
#include <Tactility/service/espnow/EspNow.h>
#include <Tactility/service/wifi/Wifi.h>

#include <esp_now.h>
#include <esp_wifi.h>

namespace tt::service::espnow {

static const auto LOGGER = Logger("EspNowService");

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
    // ESP-NOW can coexist with WiFi STA mode; only preserve WiFi state if already connected
    auto wifi_state = wifi::getRadioState();
    bool wifi_was_connected = (wifi_state == wifi::RadioState::ConnectionActive);

    // If WiFi is off or in other states, temporarily disable it to initialize ESP-NOW
    // If WiFi is already connected, keep it running and just add ESP-NOW on top
    if (!wifi_was_connected && wifi_state != wifi::RadioState::Off && wifi_state != wifi::RadioState::OffPending) {
    if (!disableWifiService()) {
        LOGGER.error("Failed to disable wifi");
        return false;
    }
    }

    wifi_mode_t mode;
    if (config.mode == Mode::Station) {
        // Use STA mode to allow coexistence with normal WiFi connection
        mode = wifi_mode_t::WIFI_MODE_STA;
    } else {
        mode = wifi_mode_t::WIFI_MODE_AP;
    }

    // Only reinitialize WiFi if it's not already running
    if (wifi::getRadioState() == wifi::RadioState::Off) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        LOGGER.error("esp_wifi_init() failed");
        return false;
    }

    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK) {
        LOGGER.error("esp_wifi_set_storage() failed");
        return false;
    }

    if (esp_wifi_set_mode(mode) != ESP_OK) {
       LOGGER.error("esp_wifi_set_mode() failed");
        return false;
    }

    if (esp_wifi_start() != ESP_OK) {
       LOGGER.error("esp_wifi_start() failed");
        return false;
    }
    }

    if (esp_wifi_set_channel(config.channel, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
       LOGGER.error("esp_wifi_set_channel() failed");
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
            LOGGER.warn("esp_wifi_set_protocol() for long range failed");
        }
    }

    LOGGER.info("WiFi initialized for ESP-NOW (preserved existing connection: {})", wifi_was_connected ? "yes" : "no");
    return true;
}

bool deinitWifi() {
    // Don't deinitialize WiFi completely - just disable ESP-NOW
    // This allows normal WiFi connection to continue
    // Only stop/deinit if WiFi was originally off

    // Since we're only using WiFi for ESP-NOW, we can safely keep it in a minimal state
    // or shut it down. For now, keep it running to support STA + ESP-NOW coexistence.

    LOGGER.info("ESP-NOW WiFi deinitialized (WiFi service continues independently)");
    return true;
}

} // namespace tt::service::espnow

#endif // ESP_PLATFORM
