#ifdef ESP_PLATFORM

#include "Tactility/service/espnow/EspNow.h"
#include "Tactility/service/espnow/EspNowService.h"

#include <Tactility/Log.h>

namespace tt::service::espnow {

constexpr const char* TAG = "EspNow";

void enable(const EspNowConfig& config) {
    auto service = findService();
    if (service != nullptr) {
        service->enable(config);
    } else {
        TT_LOG_E(TAG, "Service not found");
    }
}

void disable() {
    auto service = findService();
    if (service != nullptr) {
        service->disable();
    } else {
        TT_LOG_E(TAG, "Service not found");
    }
}

bool isEnabled() {
    auto service = findService();
    if (service != nullptr) {
        return service->isEnabled();
    } else {
        return false;
    }
}

bool addPeer(const esp_now_peer_info_t& peer) {
    auto service = findService();
    if (service != nullptr) {
        return service->addPeer(peer);
    } else {
        TT_LOG_E(TAG, "Service not found");
        return false;
    }
}

bool send(const uint8_t* address, const uint8_t* buffer, size_t bufferLength) {
    auto service = findService();
    if (service != nullptr) {
        return service->send(address, buffer, bufferLength);
    } else {
        TT_LOG_E(TAG, "Service not found");
        return false;
    }
}

ReceiverSubscription subscribeReceiver(std::function<void(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length)> onReceive) {
    auto service = findService();
    if (service != nullptr) {
        return service->subscribeReceiver(onReceive);
    } else {
        TT_LOG_E(TAG, "Service not found");
        return -1;
    }
}

void unsubscribeReceiver(ReceiverSubscription subscription) {
    auto service = findService();
    if (service != nullptr) {
        service->unsubscribeReceiver(subscription);
    } else {
        TT_LOG_E(TAG, "Service not found");
    }
}

}

#endif // ESP_PLATFORM
