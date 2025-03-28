#pragma once
#ifdef ESP_PLATFORM

#include "Tactility/MessageQueue.h"
#include "Tactility/service/Service.h"
#include "Tactility/service/espnow/EspNow.h"

#include <Tactility/Mutex.h>

#include <functional>

namespace tt::service::espnow {

class EspNowService final : public Service {

private:

    struct ReceiverSubscriptionData {
        ReceiverSubscription id;
        std::function<void(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length)> onReceive;
    };

    struct SendCallback {
        uint8_t macAddress[ESP_NOW_ETH_ALEN];
        bool success;
    };

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::vector<ReceiverSubscriptionData> subscriptions;
    ReceiverSubscription lastSubscriptionId = 0;
    bool enabled = false;

    // Dispatcher calls this and forwards to non-static function
    static void enableFromDispatcher(std::shared_ptr<void> context);
    void enableFromDispatcher(const EspNowConfig& config);

    static void disableFromDispatcher(std::shared_ptr<void> context);
    void disableFromDispatcher();

    static void receiveCallback(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length);
    void onReceive(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length);

public:

    // region Overrides

    void onStart(ServiceContext& service) override;
    void onStop(ServiceContext& service) override;

    // endregion Overrides

    // region Internal API

    void enable(const EspNowConfig& config);

    void disable();

    bool isEnabled() const;

    bool addPeer(const esp_now_peer_info_t& peer);

    bool send(const uint8_t* address, const uint8_t* buffer, size_t bufferLength);

    ReceiverSubscription subscribeReceiver(std::function<void(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length)> onReceive);

    void unsubscribeReceiver(ReceiverSubscription subscription);

    // region Internal API
};

std::shared_ptr<EspNowService> findService();

}

#endif // ESP_PLATFORM
