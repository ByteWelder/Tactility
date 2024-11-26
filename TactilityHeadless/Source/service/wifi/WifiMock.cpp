#include "Wifi.h"

#ifndef ESP_TARGET

#include "Check.h"
#include "Log.h"
#include "MessageQueue.h"
#include "Mutex.h"
#include "Pubsub.h"
#include "service/Service.h"
#include <cstdlib>
#include <cstring>

namespace tt::service::wifi {

#define TAG "wifi"
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

typedef struct {
    /** @brief Locking mechanism for modifying the Wifi instance */
    Mutex* mutex;
    /** @brief The public event bus */
    PubSub* pubsub;
    /** @brief The internal message queue */
    MessageQueue queue;
    bool scan_active;
    bool secure_connection;
    WifiRadioState radio_state;
} Wifi;


static Wifi* wifi = nullptr;

// Forward declarations
static void wifi_lock(Wifi* wifi);
static void wifi_unlock(Wifi* wifi);

// region Static

static void publish_event_simple(Wifi* wifi, WifiEventType type) {
    WifiEvent turning_on_event = {.type = type};
    tt_pubsub_publish(wifi->pubsub, &turning_on_event);
}

// endregion Static

// region Alloc

static Wifi* wifi_alloc() {
    auto* instance = static_cast<Wifi*>(malloc(sizeof(Wifi)));
    instance->mutex = tt_mutex_alloc(MutexTypeRecursive);
    instance->pubsub = tt_pubsub_alloc();
    instance->scan_active = false;
    instance->radio_state = WIFI_RADIO_CONNECTION_ACTIVE;
    instance->secure_connection = false;
    return instance;
}

static void wifi_free(Wifi* instance) {
    tt_mutex_free(instance->mutex);
    tt_pubsub_free(instance->pubsub);
    free(instance);
}

// endregion Alloc

// region Public functions

PubSub* get_pubsub() {
    tt_assert(wifi);
    return wifi->pubsub;
}

WifiRadioState get_radio_state() {
    return wifi->radio_state;
}

void scan() {
    tt_assert(wifi);
    wifi->scan_active = false; // TODO: enable and then later disable automatically
}

bool is_scanning() {
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

void set_scan_records(uint16_t records) {
    tt_assert(wifi);
    // TODO: implement
}

void get_scan_results(WifiApRecord records[], uint16_t limit, uint16_t* result_count) {
    tt_check(wifi);
    tt_check(result_count);

    if (limit >= 5) {
        strcpy((char*)records[0].ssid, "Home WiFi");
        records[0].auth_mode = WIFI_AUTH_WPA2_PSK;
        records[0].rssi = -30;
        strcpy((char*)records[1].ssid, "Living Room");
        records[1].auth_mode = WIFI_AUTH_WPA2_PSK;
        records[1].rssi = -67;
        strcpy((char*)records[2].ssid, "No place like 127.0.0.1");
        records[2].auth_mode = WIFI_AUTH_WPA2_PSK;
        records[2].rssi = -70;
        strcpy((char*)records[3].ssid, "Public Wi-Fi");
        records[3].auth_mode = WIFI_AUTH_WPA2_PSK;
        records[3].rssi = -80;
        strcpy((char*)records[4].ssid, "Bad Reception");
        records[4].auth_mode = WIFI_AUTH_WPA2_PSK;
        records[4].rssi = -90;
        *result_count = 5;
    } else {
        *result_count = 0;
    }
}

void set_enabled(bool enabled) {
    tt_assert(wifi != NULL);
    if (enabled) {
        wifi->radio_state = WIFI_RADIO_ON;
        wifi->secure_connection = true;
    } else {
        wifi->radio_state = WIFI_RADIO_OFF;
    }
}

bool is_connection_secure() {
    return wifi->secure_connection;
}

int get_rssi() {
    if (wifi->radio_state == WIFI_RADIO_CONNECTION_ACTIVE) {
        return -30;
    } else {
        return 0;
    }
}

// endregion Public functions

static void lock(Wifi* wifi) {
    tt_crash("this fails for now");
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_acquire(wifi->mutex, 100);
}

static void unlock(Wifi* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_release(wifi->mutex);
}


static void service_start(TT_UNUSED Service& service) {
    tt_check(wifi == nullptr);
    wifi = wifi_alloc();
}

static void service_stop(TT_UNUSED Service& service) {
    tt_check(wifi != nullptr);

    wifi_free(wifi);
    wifi = nullptr;
}

void wifi_main(void*) {
    // NO-OP
}

extern const Manifest manifest = {
    .id = "Wifi",
    .onStart = &service_start,
    .onStop = &service_stop
};

} // namespace

#endif // ESP_TARGET