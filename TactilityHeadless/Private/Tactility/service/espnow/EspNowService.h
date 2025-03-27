#pragma once
#ifdef ESP_PLATFORM

#include "Tactility/MessageQueue.h"
#include "Tactility/service/Service.h"
#include "Tactility/service/espnow/EspNow.h"

#include <Tactility/Mutex.h>

#include <functional>

namespace tt::service::espnow {

constexpr uint32_t QUEUE_SIZE = 6;

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

    struct ReceiveCallback {
        uint8_t macAddress[ESP_NOW_ETH_ALEN];
        uint8_t* data;
        uint16_t dataLength;
    };

    enum class EventId {
        SendCallback,
        ReceiveCallback,
    };

    union EventInfo {
        SendCallback sendCallback;
        ReceiveCallback receiveCallback;
    };

    struct EspNowEvent {
        EventId id;
        EventInfo info;
    };

    MessageQueue queue = MessageQueue(QUEUE_SIZE, sizeof(EspNowEvent));
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::vector<ReceiverSubscriptionData> subscriptions;
    ReceiverSubscription lastSubscriptionId = 0;
    bool enabled = false;

    // Dispatcher calls this and forwards to non-static function
    static void enableFromDispatcher(std::shared_ptr<void> context);
    void enableFromDispatcher(const EspNowConfig& config);

    static void disableFromDispatcher(std::shared_ptr<void> context);
    void disableFromDispatcher();

    static void sendCallback(const uint8_t* macAddress, esp_now_send_status_t status);
    static void receiveCallback(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length);

    void onSend(const uint8_t* macAddress, esp_now_send_status_t status);
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

    bool send(const uint8_t* address, const uint8_t* buffer, size_t bufferLength);

    ReceiverSubscription subscribeReceiver(std::function<void(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length)> onReceive);

    void unsubscribeReceiver(ReceiverSubscription subscription);

    // region Internal API
};

std::shared_ptr<EspNowService> findService();

}

#endif // ESP_PLATFORM
