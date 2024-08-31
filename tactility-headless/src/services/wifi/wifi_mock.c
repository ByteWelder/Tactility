#include "wifi.h"

#ifndef ESP_TARGET

#include "assets.h"
#include "check.h"
#include "log.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "service.h"
#include <sys/cdefs.h>

#define TAG "wifi"
#define WIFI_SCAN_RECORD_LIMIT 16 // default, can be overridden
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

typedef struct {
    /** @brief Locking mechanism for modifying the Wifi instance */
    Mutex* mutex;
    /** @brief The public event bus */
    PubSub* pubsub;
    /** @brief The internal message queue */
    MessageQueue* queue;
    bool scan_active;
    bool secure_connection;
    WifiRadioState radio_state;
} Wifi;


static Wifi* wifi_singleton = NULL;

// Forward declarations
static void wifi_lock(Wifi* wifi);
static void wifi_unlock(Wifi* wifi);

// region Alloc

static Wifi* wifi_alloc() {
    Wifi* instance = malloc(sizeof(Wifi));
    instance->mutex = tt_mutex_alloc(MutexTypeRecursive);
    instance->pubsub = tt_pubsub_alloc();
    instance->scan_active = false;
    instance->radio_state = WIFI_RADIO_OFF;
    instance->secure_connection = false;
    return instance;
}

static void wifi_free(Wifi* instance) {
    tt_mutex_free(instance->mutex);
    tt_pubsub_free(instance->pubsub);
    tt_message_queue_free(instance->queue);
    free(instance);
}

// endregion Alloc

// region Public functions

PubSub* wifi_get_pubsub() {
    tt_assert(wifi_singleton);
    return wifi_singleton->pubsub;
}

WifiRadioState wifi_get_radio_state() {
    return wifi_singleton->radio_state;
}

void wifi_scan() {
    tt_assert(wifi_singleton);
    wifi_singleton->scan_active = false; // TODO: enable and then later disable automatically
}

bool wifi_is_scanning() {
    tt_assert(wifi_singleton);
    return wifi_singleton->scan_active;
}

void wifi_connect(const char* ssid, const char _Nullable password[64]) {
    tt_assert(wifi_singleton);
    tt_check(strlen(ssid) <= 32);
    // TODO: implement
}

void wifi_disconnect() {
    tt_assert(wifi_singleton);
    // TODO: implement
}

void wifi_set_scan_records(uint16_t records) {
    tt_assert(wifi_singleton);
    // TODO: implement
}

void wifi_get_scan_results(WifiApRecord records[], uint16_t limit, uint16_t* result_count) {
    tt_check(wifi_singleton);
    tt_check(result_count);

    // TODO: implement
    *result_count = 0;
}

void wifi_set_enabled(bool enabled) {
    tt_assert(wifi_singleton != NULL);
    if (enabled) {
        wifi_singleton->radio_state = WIFI_RADIO_CONNECTION_ACTIVE;
        wifi_singleton->secure_connection = true;
    } else {
        wifi_singleton->radio_state = WIFI_RADIO_OFF;
    }
}

bool wifi_is_connection_secure() {
    return wifi_singleton->secure_connection;
}

int wifi_get_rssi() {
    // TODO: implement
    return -10;
}

// endregion Public functions

static void wifi_lock(Wifi* wifi) {
    tt_crash("this fails for now");
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_acquire(wifi->mutex, 100);
}

static void wifi_unlock(Wifi* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_release(wifi->mutex);
}


static void wifi_service_start(TT_UNUSED Service service) {
    tt_check(wifi_singleton == NULL);
    wifi_singleton = wifi_alloc();
}

static void wifi_service_stop(TT_UNUSED Service service) {
    tt_check(wifi_singleton != NULL);

    wifi_free(wifi_singleton);
    wifi_singleton = NULL;
}

const ServiceManifest wifi_service = {
    .id = "wifi",
    .on_start = &wifi_service_start,
    .on_stop = &wifi_service_stop
};

#endif // ESP_TARGET