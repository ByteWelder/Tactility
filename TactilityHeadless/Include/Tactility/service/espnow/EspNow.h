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

void enable(const EspNowConfig& config);

void disable();

bool isEnabled();

bool send(const uint8_t* address, const uint8_t* buffer, size_t bufferLength);

ReceiverSubscription subscribeReceiver(std::function<void(const uint8_t* buffer, size_t bufferLength)> onReceive);

void unsubscribeReceiver(ReceiverSubscription subscription);

}

#endif // ESP_PLATFORM