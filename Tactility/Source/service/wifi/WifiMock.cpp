#include "Tactility/service/wifi/Wifi.h"

#ifndef ESP_PLATFORM

#include "Tactility/service/ServiceContext.h"

#include <Tactility/Check.h>
#include <Tactility/Log.h>
#include <Tactility/Mutex.h>
#include <Tactility/PubSub.h>

namespace tt::service::wifi {

#define TAG "wifi"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

struct Wifi {
    Wifi() = default;
    ~Wifi() = default;

    /** @brief Locking mechanism for modifying the Wifi instance */
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    /** @brief The public event bus */
    std::shared_ptr<PubSub> pubsub = std::make_shared<PubSub>();
    /** @brief The internal message queue */
    bool scan_active = false;
    bool secure_connection = false;
    RadioState radio_state = RadioState::ConnectionActive;
};


static Wifi* wifi = nullptr;

// region Static

static void publish_event_simple(Wifi* wifi, EventType type) {
    Event turning_on_event = { .type = type };
    wifi->pubsub->publish(&turning_on_event);
}

// endregion Static

// region Public functions

std::shared_ptr<PubSub> getPubsub() {
    assert(wifi);
    return wifi->pubsub;
}

RadioState getRadioState() {
    return wifi->radio_state;
}

std::string getConnectionTarget() {
    return "Home Wifi";
}

void scan() {
    assert(wifi);
    wifi->scan_active = false; // TODO: enable and then later disable automatically
}

bool isScanning() {
    assert(wifi);
    return wifi->scan_active;
}

void connect(const settings::WifiApSettings* ap, bool remember) {
    assert(wifi);
    // TODO: implement
}

void disconnect() {
    assert(wifi);
}

void setScanRecords(uint16_t records) {
    assert(wifi);
    // TODO: implement
}

std::vector<ApRecord> getScanResults() {
    tt_check(wifi);

    std::vector<ApRecord> records;
    records.push_back((ApRecord) {
        .ssid = "Home Wifi",
        .rssi = -30,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((ApRecord) {
        .ssid = "No place like 127.0.0.1",
        .rssi = -67,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((ApRecord) {
        .ssid = "Pretty fly for a Wi-Fi",
        .rssi = -70,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((ApRecord) {
        .ssid = "An AP with a really, really long name",
        .rssi = -80,
        .auth_mode = WIFI_AUTH_WPA2_PSK
    });
    records.push_back((ApRecord) {
        .ssid = "Bad Reception",
        .rssi = -90,
        .auth_mode = WIFI_AUTH_OPEN
    });

    return records;
}

void setEnabled(bool enabled) {
    assert(wifi != nullptr);
    if (enabled) {
        wifi->radio_state = RadioState::On;
        wifi->secure_connection = true;
    } else {
        wifi->radio_state = RadioState::Off;
    }
}

bool isConnectionSecure() {
    return wifi->secure_connection;
}

int getRssi() {
    if (wifi->radio_state == RadioState::ConnectionActive) {
        return -30;
    } else {
        return 0;
    }
}

// endregion Public functions

class WifiService final : public Service {

public:

    void onStart(TT_UNUSED ServiceContext& service) final {
        tt_check(wifi == nullptr);
        wifi = new Wifi();
    }

    void onStop(TT_UNUSED ServiceContext& service) final {
        tt_check(wifi != nullptr);
        delete wifi;
        wifi = nullptr;
    }
};

extern const ServiceManifest manifest = {
    .id = "Wifi",
    .createService = create<WifiService>
};

} // namespace

#endif // ESP_PLATFORM