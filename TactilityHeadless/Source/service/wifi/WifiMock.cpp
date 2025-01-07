#include "Wifi.h"

#ifndef ESP_TARGET

#include "Check.h"
#include "Log.h"
#include "MessageQueue.h"
#include "Mutex.h"
#include "Pubsub.h"
#include "service/ServiceContext.h"

namespace tt::service::wifi {

#define TAG "wifi"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

struct Wifi {
    Wifi() = default;
    ~Wifi() = default;

    /** @brief Locking mechanism for modifying the Wifi instance */
    Mutex mutex = Mutex(Mutex::TypeRecursive);
    /** @brief The public event bus */
    std::shared_ptr<PubSub> pubsub = std::make_shared<PubSub>();
    /** @brief The internal message queue */
    bool scan_active = false;
    bool secure_connection = false;
    WifiRadioState radio_state = WIFI_RADIO_CONNECTION_ACTIVE;
};


static Wifi* wifi = nullptr;

// region Static

static void publish_event_simple(Wifi* wifi, WifiEventType type) {
    WifiEvent turning_on_event = {.type = type};
    tt_pubsub_publish(wifi->pubsub, &turning_on_event);
}

// endregion Static

// region Public functions

std::shared_ptr<PubSub> getPubsub() {
    tt_assert(wifi);
    return wifi->pubsub;
}

WifiRadioState getRadioState() {
    return wifi->radio_state;
}

std::string getConnectionTarget() {
    return "Home Wifi";
}

void scan() {
    tt_assert(wifi);
    wifi->scan_active = false; // TODO: enable and then later disable automatically
}

bool isScanning() {
    tt_assert(wifi);
    return wifi->scan_active;
}

void connect(const settings::WifiApSettings* ap, bool remember) {
    tt_assert(wifi);
    // TODO: implement
}

void disconnect() {
    tt_assert(wifi);
}

void setScanRecords(uint16_t records) {
    tt_assert(wifi);
    // TODO: implement
}

std::vector<WifiApRecord> getScanResults() {
    tt_check(wifi);

    std::vector<WifiApRecord> records;
    records.push_back((WifiApRecord) {
        .ssid = "Home Wifi",
        .rssi = -30,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((WifiApRecord) {
        .ssid = "No place like 127.0.0.1",
        .rssi = -67,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((WifiApRecord) {
        .ssid = "Pretty fly for a Wi-Fi",
        .rssi = -70,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((WifiApRecord) {
        .ssid = "An AP with a really, really long name",
        .rssi = -80,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((WifiApRecord) {
        .ssid = "Bad Reception",
        .rssi = -90,
        .auth_mode = WIFI_AUTH_OPEN
    });

    return records;
}

void setEnabled(bool enabled) {
    tt_assert(wifi != nullptr);
    if (enabled) {
        wifi->radio_state = WIFI_RADIO_ON;
        wifi->secure_connection = true;
    } else {
        wifi->radio_state = WIFI_RADIO_OFF;
    }
}

bool isConnectionSecure() {
    return wifi->secure_connection;
}

int getRssi() {
    if (wifi->radio_state == WIFI_RADIO_CONNECTION_ACTIVE) {
        return -30;
    } else {
        return 0;
    }
}

// endregion Public functions

static void service_start(TT_UNUSED ServiceContext& service) {
    tt_check(wifi == nullptr);
    wifi = new Wifi();
}

static void service_stop(TT_UNUSED ServiceContext& service) {
    tt_check(wifi != nullptr);
    delete wifi;
    wifi = nullptr;
}

void wifi_main(void*) {
    // NO-OP
}

extern const ServiceManifest manifest = {
    .id = "Wifi",
    .onStart = &service_start,
    .onStop = &service_stop
};

} // namespace

#endif // ESP_TARGET