#pragma once

#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

#if defined(CONFIG_SOC_WIFI_SUPPORTED) || defined(CONFIG_ESP_HOSTED_ENABLED)

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <esp_now.h>

namespace espnow {

enum class Mode {
    Station,
    AccessPoint
};

struct EspNowConfig {
    uint8_t masterKey[ESP_NOW_KEY_LEN];
    Mode mode;
    uint8_t channel;
    bool longRange;
    bool encrypt;

    EspNowConfig(
        const uint8_t masterKey[ESP_NOW_KEY_LEN],
        Mode mode,
        uint8_t channel,
        bool longRange,
        bool encrypt
    ) : mode(mode), channel(channel), longRange(longRange), encrypt(encrypt) {
        memcpy(this->masterKey, masterKey, ESP_NOW_KEY_LEN);
    }
};

}

namespace espnow::backend {

/**
 * Bring up the backend (WiFi if needed + ESP-NOW init + recv/send callbacks + PMK).
 * Native backend calls esp_now_* directly; hosted backend bridges over esp_hosted's
 * custom RPC channel to the co-processor. Either way, recvCallback is invoked with
 * the same signature ESP-NOW itself uses.
 */
bool init(const EspNowConfig& config, esp_now_recv_cb_t recvCallback);

bool deinit();

bool addPeer(const esp_now_peer_info_t& peer);

bool send(const uint8_t* address, const uint8_t* buffer, size_t bufferLength);

/** Returns 0 if not initialized. */
uint32_t getVersion();

}

#endif // CONFIG_SOC_WIFI_SUPPORTED || CONFIG_ESP_HOSTED_ENABLED
