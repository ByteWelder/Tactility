#ifdef ESP_PLATFORM

#include "Tactility/service/espnow/EspNowService.h"
#include "Tactility/TactilityHeadless.h"
#include "Tactility/service/ServiceManifest.h"
#include "Tactility/service/ServiceRegistry.h"
#include "Tactility/service/espnow/EspNowWifi.h"
#include <cstring>
#include <esp_now.h>
#include <esp_random.h>

namespace tt::service::espnow {

extern const ServiceManifest manifest;

constexpr const char* TAG = "EspNowService";
constexpr TickType_t MAX_DELAY = 1000U / portTICK_PERIOD_MS;
static uint8_t BROADCAST_MAC[ESP_NOW_ETH_ALEN];

constexpr bool isBroadcastAddress(uint8_t address[ESP_NOW_ETH_ALEN]) { return memcmp(address, BROADCAST_MAC, ESP_NOW_ETH_ALEN) == 0; }

void EspNowService::onStart(ServiceContext& service) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    memset(BROADCAST_MAC, 0xFF, sizeof(BROADCAST_MAC));
}

void EspNowService::onStop(ServiceContext& service) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (isEnabled()) {
        disable();
    }
}

int32_t EspNowService::threadMainCallback(void* context) {
    auto* service = (EspNowService*)context;
    return service->threadMain();
}

int32_t EspNowService::threadMain() {
    EspNowEvent event;

    while (!isThreadInterruptRequested()) {
        if (queue.get(&event, 50 / portTICK_PERIOD_MS)) {
            TT_LOG_E(TAG, "Queue: event %d processed", (int)event.id);
            // TODO: Send events to subscribers
        }
    }

    TT_LOG_E(TAG, "Thread finished");
    return 0;
}

// region Enable

void EspNowService::enable(const EspNowConfig& config) {
    auto enable_context = std::make_shared<EspNowConfig>(config);
    getMainDispatcher().dispatch(enableFromDispatcher, enable_context);
}

void EspNowService::enableFromDispatcher(std::shared_ptr<void> context) {
    auto service = findService();
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not running");
        return;
    }

    auto config = std::static_pointer_cast<EspNowConfig>(context);

    service->enableFromDispatcher(*config);
}

void EspNowService::enableFromDispatcher(const EspNowConfig& config) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (!initWifi(config)) {
        TT_LOG_E(TAG, "initWifi() failed");
        return;
    }

    if (esp_now_init() != ESP_OK) {
        TT_LOG_E(TAG, "esp_now_init() failed");
        return;
    }

    if (esp_now_register_send_cb(sendCallback) != ESP_OK) {
        TT_LOG_E(TAG, "esp_now_register_send_cb() failed");
        return;
    }

    if (esp_now_register_recv_cb(receiveCallback) != ESP_OK) {
        TT_LOG_E(TAG, "esp_now_register_recv_cb() failed");
        return;
    }

//#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
//    ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
//    ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
//#endif

    if (esp_now_set_pmk(config.masterKey) != ESP_OK) {
        TT_LOG_E(TAG, "esp_now_set_pmk() failed");
        return;
    }

    // Add broadcast peer information to peer list
    auto* peer = (esp_now_peer_info_t*)malloc(sizeof(esp_now_peer_info_t));
    if (peer == nullptr) {
        TT_LOG_E(TAG, "Out of memory");
        esp_now_deinit();
        return;
    }

    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = config.channel;
    if (config.mode == Mode::Station) {
        peer->ifidx = WIFI_IF_STA;
    } else {
        peer->ifidx = WIFI_IF_AP;
    }
    peer->encrypt = config.encrypt;
    memcpy(peer->peer_addr, BROADCAST_MAC, ESP_NOW_ETH_ALEN);
    if (esp_now_add_peer(peer) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to add peer");
    }
    free(peer);

    thread = std::make_unique<Thread>(
        "EspNow",
        4096,
        threadMainCallback,
        this,
        (int)Thread::Priority::Higher
    );

    thread->start();
}

// endregion Enable

// region Disable

void EspNowService::disable() {
    getMainDispatcher().dispatch(disableFromDispatcher, nullptr);
}

void EspNowService::disableFromDispatcher(TT_UNUSED std::shared_ptr<void> context) {
    auto service = findService();
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not running");
        return;
    }

    service->disableFromDispatcher();
}

void EspNowService::disableFromDispatcher() {
    auto lock = mutex.asScopedLock();

    lock.lock();
    threadInterruptRequested = true;
    lock.unlock();

    thread->join();
    lock.lock();

    if (esp_now_deinit() != ESP_OK) {
        TT_LOG_E(TAG, "esp_now_deinit() failed");
    }

    if (!deinitWifi()) {
        TT_LOG_E(TAG, "deinitWifi() failed");
    }
}

// region Disable

// region Callbacks

void EspNowService::sendCallback(const uint8_t* macAddress, esp_now_send_status_t status) {
    auto service = findService();
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not running");
        return;
    }
    service->onSend(macAddress, status);
}

void EspNowService::receiveCallback(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length) {
    auto service = findService();
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not running");
        return;
    }
    service->onReceive(receiveInfo, data, length);
}

void EspNowService::onSend(const uint8_t* macAddress, esp_now_send_status_t status) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    EspNowEvent event;
    SendCallback* callback = &event.info.sendCallback;

    if (macAddress == nullptr) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    event.id = EventId::SendCallback;
    memcpy(callback->macAddress, macAddress, ESP_NOW_ETH_ALEN);
    callback->success = (status == ESP_NOW_SEND_SUCCESS);

    if (!queue.put(&event, MAX_DELAY)) {
        TT_LOG_E(TAG, "Failed to queue onSend() data");
    }
}

void EspNowService::onReceive(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    EspNowEvent event;
    ReceiveCallback* callback = &event.info.receiveCallback;
    uint8_t* source_address = receiveInfo->src_addr;
    uint8_t* destination_address = receiveInfo->des_addr;

    if (source_address == nullptr || data == nullptr || length <= 0) {
        TT_LOG_E(TAG, "Receive callback argument error");
        return;
    }

    if (isBroadcastAddress(destination_address)) {
        /* If added a peer with encryption before, the receive packets may be
         * encrypted as peer-to-peer message or unencrypted over the broadcast channel.
         * Users can check the destination address to distinguish it.
         */
        TT_LOG_D(TAG, "Receive broadcast ESPNOW data");
    } else {
        TT_LOG_D(TAG, "Receive unicast ESPNOW data");
    }

    event.id = EventId::ReceiveCallback;
    memcpy(callback->macAddress, source_address, ESP_NOW_ETH_ALEN);
    callback->data = (uint8_t*)malloc(length);
    if (callback->data == nullptr) {
        TT_LOG_E(TAG, "Malloc receive data fail");
        return;
    }

    memcpy(callback->data, data, length);
    callback->dataLength = length;

    if (!queue.put(&event, MAX_DELAY)) {
        TT_LOG_W(TAG, "Send receive queue fail");
        free(callback->data);
    }
}

// endregion Callbacks

bool EspNowService::isEnabled() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return thread != nullptr;
}

bool EspNowService::send(const uint8_t* address, const uint8_t* buffer, size_t bufferLength) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (!isEnabled()) {
        return false;
    } else {
        return esp_now_send(address, buffer, bufferLength) == ESP_OK;
    }
}

ReceiverSubscription EspNowService::subscribeReceiver(std::function<void(const uint8_t* buffer, size_t bufferSize)> onReceive) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    auto id = lastSubscriptionId++;

    subscriptions.push_back(ReceiverSubscriptionData {
        .id = id,
        .onReceive = onReceive
    });

    return id;
}

void EspNowService::unsubscribeReceiver(tt::service::espnow::ReceiverSubscription subscriptionId) {
    auto lock = mutex.asScopedLock();
    lock.lock();
    std::erase_if(subscriptions, [subscriptionId](auto& subscription) { return subscription.id == subscriptionId; });
}

std::shared_ptr<EspNowService> findService() {
    return std::static_pointer_cast<EspNowService>(
        service::findServiceById(manifest.id)
    );
}

extern const ServiceManifest manifest = {
    .id = "EspNow",
    .createService = create<EspNowService>
};

}

#endif // ESP_PLATFORM
