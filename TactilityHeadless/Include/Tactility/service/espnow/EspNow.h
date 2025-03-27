#pragma once
#ifdef ESP_PLATFORM

#include <cstdint>
#include <cstring>
#include <esp_now.h>
#include <functional>

namespace tt::service::espnow {

typedef int ReceiverSubscription;
constexpr ReceiverSubscription NO_SUBSCRIPTION = -1;

enum class Mode {
    Station,
    AccessPoint
};

struct EspNowConfig {
    uint8_t masterKey[16];
    Mode mode;
    uint8_t channel;
    bool longRange;
    bool encrypt;

    EspNowConfig(
        uint8_t masterKey[16],
        Mode mode,
        uint8_t channel,
        bool longRange,
        bool encrypt
    ) : mode(mode), channel(channel), longRange(longRange), encrypt(encrypt) {
        memcpy((void*)this->masterKey, (void*)masterKey, 16);
    }
};

// TODO: Update
// From https://github.com/espressif/esp-idf/blob/master/examples/wifi/espnow/main/espnow_example.h#L80
typedef struct EspNowData {
    uint8_t type;                         // Broadcast or unicast ESPNOW data.
    uint8_t state;                        // Indicate that if has received broadcast ESPNOW data or not.
    uint16_t seq_num;                     // Sequence number of ESPNOW data.
    uint16_t crc;                         // CRC16 value of ESPNOW data.
    uint32_t magic;                       // Magic number which is used to determine which device to send unicast ESPNOW data.
    uint8_t payload[0];                   // Real payload of ESPNOW data.
} __attribute__((packed));

// TODO: Update
// From https://github.com/espressif/esp-idf/blob/master/examples/wifi/espnow/main/espnow_example.h#L80
typedef struct EspNowSendParam {
    bool unicast;                         // Send unicast ESPNOW data.
    bool broadcast;                       // Send broadcast ESPNOW data.
    uint8_t state;                        // Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       // Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       // Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       // Delay between sending two ESPNOW data, unit: ms.
    int length;                           // Length of ESPNOW data to be sent, unit: byte.
    uint8_t* buffer;                      // Buffer pointing to ESPNOW data.
    uint8_t destinationMac[ESP_NOW_ETH_ALEN]; // MAC address of destination device.
};

void enable(const EspNowConfig& config);

void disable();

bool isEnabled();

bool send(const uint8_t* address, const uint8_t* buffer, size_t bufferLength);

ReceiverSubscription subscribeReceiver(std::function<void(const uint8_t* buffer, size_t bufferLength)> onReceive);

void unsubscribeReceiver(ReceiverSubscription subscription);

}

#endif // ESP_PLATFORM