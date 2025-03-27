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

    if (enabled) {
        return;
    }

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

    enabled = true;
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

    if (!enabled) {
        return;
    }

    if (esp_now_deinit() != ESP_OK) {
        TT_LOG_E(TAG, "esp_now_deinit() failed");
    }

    if (!deinitWifi()) {
        TT_LOG_E(TAG, "deinitWifi() failed");
    }

    enabled = false;
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

    for (const auto& item: subscriptions) {
        item.onReceive(receiveInfo, data, length);
    }
}

// endregion Callbacks

bool EspNowService::isEnabled() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return enabled;
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

ReceiverSubscription EspNowService::subscribeReceiver(std::function<void(const esp_now_recv_info_t* receiveInfo, const uint8_t* data, int length)> onReceive) {
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
