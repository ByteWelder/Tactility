#ifndef ESP_PLATFORM

#include "wifi.h"

#include "check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
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
    /** @brief Scanning results */
    wifi_ap_record_t* _Nullable scan_list;
    /** @brief The current item count in scan_list (-1 when scan_list is NULL) */
    uint16_t scan_list_count;
    /** @brief Maximum amount of records to scan (value > 0) */
    uint16_t scan_list_limit;
    bool scan_active;
    EventGroupHandle_t event_group;
    WifiRadioState radio_state;
} Wifi;

typedef enum {
    WifiMessageTypeRadioOn,
    WifiMessageTypeRadioOff,
    WifiMessageTypeScan,
    WifiMessageTypeConnect,
    WifiMessageTypeDisconnect
} WifiMessageType;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} WifiConnectMessage;

typedef struct {
    WifiMessageType type;
    union {
        WifiConnectMessage connect_message;
    };
} WifiMessage;

static Wifi* wifi_singleton = NULL;

// Forward declarations
static void wifi_scan_list_free_safely(Wifi* wifi);
static void wifi_disconnect_internal(Wifi* wifi);
static void wifi_lock(Wifi* wifi);
static void wifi_unlock(Wifi* wifi);

// region Alloc

static Wifi* wifi_alloc() {
    Wifi* instance = malloc(sizeof(Wifi));
    instance->mutex = tt_mutex_alloc(MutexTypeRecursive);
    instance->pubsub = tt_pubsub_alloc();
    // TODO: Deal with messages that come in while an action is ongoing
    // for example: when scanning and you turn off the radio, the scan should probably stop or turning off
    // the radio should disable the on/off button in the app as it is pending.
    instance->queue = tt_message_queue_alloc(1, sizeof(WifiMessage));
    instance->scan_active = false;
    instance->scan_list = NULL;
    instance->scan_list_count = 0;
    instance->scan_list_limit = WIFI_SCAN_RECORD_LIMIT;
    instance->event_group = xEventGroupCreate();
    instance->radio_state = WIFI_RADIO_OFF;
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
    WifiMessage message = {.type = WifiMessageTypeScan};
    // No need to lock for queue
    tt_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
}

bool wifi_is_scanning() {
    tt_assert(wifi_singleton);
    return wifi_singleton->scan_active;
}

void wifi_connect(const char* ssid, const char _Nullable password[64]) {
    tt_assert(wifi_singleton);
    tt_check(strlen(ssid) <= 32);
    WifiMessage message = {.type = WifiMessageTypeConnect};
    memcpy(message.connect_message.ssid, ssid, 32);
    if (password != NULL) {
        memcpy(message.connect_message.password, password, 64);
    } else {
        message.connect_message.password[0] = 0;
    }
    tt_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
}

void wifi_disconnect() {
    tt_assert(wifi_singleton);
    WifiMessage message = {.type = WifiMessageTypeDisconnect};
    tt_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
}

void wifi_set_scan_records(uint16_t records) {
    tt_assert(wifi_singleton);
    if (records != wifi_singleton->scan_list_limit) {
        wifi_scan_list_free_safely(wifi_singleton);
        wifi_singleton->scan_list_limit = records;
    }
}

void wifi_get_scan_results(WifiApRecord records[], uint16_t limit, uint16_t* result_count) {
    tt_check(wifi_singleton);
    tt_check(result_count);

    if (wifi_singleton->scan_list_count == 0) {
        *result_count = 0;
    } else {
        uint16_t i = 0;
        TT_LOG_I(TAG, "processing up to %d APs", wifi_singleton->scan_list_count);
        uint16_t last_index = MIN(wifi_singleton->scan_list_count, limit);
        for (; i < last_index; ++i) {
            memcpy(records[i].ssid, wifi_singleton->scan_list[i].ssid, 33);
            records[i].rssi = wifi_singleton->scan_list[i].rssi;
            records[i].auth_mode = wifi_singleton->scan_list[i].authmode;
        }
        // The index already overflowed right before the for-loop was terminated,
        // so it effectively became the list count:
        *result_count = i;
    }
}

void wifi_set_enabled(bool enabled) {
    if (enabled) {
        WifiMessage message = {.type = WifiMessageTypeRadioOn};
        // No need to lock for queue
        tt_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
    } else {
        WifiMessage message = {.type = WifiMessageTypeRadioOff};
        // No need to lock for queue
        tt_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
    }
}

// endregion Public functions

static void wifi_lock(Wifi* wifi) {
    tt_crash("this fails for now");
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_check(xSemaphoreTakeRecursive(wifi->mutex, portMAX_DELAY) == pdPASS);
}

static void wifi_unlock(Wifi* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_check(xSemaphoreGiveRecursive(wifi->mutex) == pdPASS);
}

static void wifi_scan_list_alloc(Wifi* wifi) {
    tt_check(wifi->scan_list == NULL);
    wifi->scan_list = malloc(sizeof(wifi_ap_record_t) * wifi->scan_list_limit);
    wifi->scan_list_count = 0;
}

static void wifi_scan_list_alloc_safely(Wifi* wifi) {
    if (wifi->scan_list == NULL) {
        wifi_scan_list_alloc(wifi);
    }
}

static void wifi_scan_list_free(Wifi* wifi) {
    tt_check(wifi->scan_list != NULL);
    free(wifi->scan_list);
    wifi->scan_list = NULL;
    wifi->scan_list_count = 0;
}

static void wifi_scan_list_free_safely(Wifi* wifi) {
    if (wifi->scan_list != NULL) {
        wifi_scan_list_free(wifi);
    }
}

static void wifi_publish_event_simple(Wifi* wifi, WifiEventType type) {
    WifiEvent turning_on_event = {.type = type};
    tt_pubsub_publish(wifi->pubsub, &turning_on_event);
}

static void wifi_enable(Wifi* wifi) {
    WifiRadioState state = wifi->radio_state;
    if (
        state == WIFI_RADIO_ON ||
        state == WIFI_RADIO_ON_PENDING ||
        state == WIFI_RADIO_OFF_PENDING
    ) {
        TT_LOG_W(TAG, "Can't enable from current state");
        return;
    }

    wifi->radio_state = WIFI_RADIO_ON;
    wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOn);
    TT_LOG_I(TAG, "Enabled");
}

static void wifi_disable(Wifi* wifi) {
    WifiRadioState state = wifi->radio_state;
    if (
        state == WIFI_RADIO_OFF ||
        state == WIFI_RADIO_OFF_PENDING ||
        state == WIFI_RADIO_ON_PENDING
    ) {
        TT_LOG_W(TAG, "Can't disable from current state");
        return;
    }

    wifi->radio_state = WIFI_RADIO_OFF;
    wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
    TT_LOG_I(TAG, "Disabled");
}

static void wifi_scan_internal(Wifi* wifi) {
    WifiRadioState state = wifi->radio_state;
    if (state != WIFI_RADIO_ON && state != WIFI_RADIO_CONNECTION_ACTIVE) {
        TT_LOG_W(TAG, "Scan unavailable: wifi not enabled");
        return;
    }

    TT_LOG_I(TAG, "Starting scan");
    wifi->scan_active = true;
    wifi_publish_event_simple(wifi, WifiEventTypeScanStarted);

    // TODO: fake entries

    wifi_publish_event_simple(wifi, WifiEventTypeScanFinished);
    wifi->scan_active = false;
    TT_LOG_I(TAG, "Finished scan");
}

static void wifi_connect_internal(Wifi* wifi, WifiConnectMessage* connect_message) {
    wifi_disconnect_internal(wifi);

    wifi->radio_state = WIFI_RADIO_CONNECTION_PENDING;
    wifi_publish_event_simple(wifi, WifiEventTypeConnectionPending);

    // TODO: Sleep?

    wifi->radio_state = WIFI_RADIO_CONNECTION_ACTIVE;
    wifi_publish_event_simple(wifi, WifiEventTypeConnectionSuccess);

    TT_LOG_I(TAG, "Connected to %s", connect_message->ssid);
}

static void wifi_disconnect_internal(Wifi* wifi) {
    wifi->radio_state = WIFI_RADIO_ON;
    wifi_publish_event_simple(wifi, WifiEventTypeDisconnected);
    TT_LOG_I(TAG, "Disconnected");
}

static void wifi_disconnect_internal_but_keep_active(Wifi* wifi) {
    wifi->radio_state = WIFI_RADIO_ON;
    wifi_publish_event_simple(wifi, WifiEventTypeDisconnected);
    TT_LOG_I(TAG, "Disconnected");
}

// ESP wifi APIs need to run from the main task, so we can't just spawn a thread
_Noreturn int32_t wifi_main(void* p) {
    UNUSED(p);

    TT_LOG_I(TAG, "Started main loop");
    tt_check(wifi_singleton != NULL);
    Wifi* wifi = wifi_singleton;
    MessageQueue* queue = wifi->queue;

    WifiMessage message;
    while (true) {
        if (tt_message_queue_get(queue, &message, 1000 / portTICK_PERIOD_MS) == TtStatusOk) {
            TT_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case WifiMessageTypeRadioOn:
                    wifi_enable(wifi);
                    break;
                case WifiMessageTypeRadioOff:
                    wifi_disable(wifi);
                    break;
                case WifiMessageTypeScan:
                    wifi_scan_internal(wifi);
                    break;
                case WifiMessageTypeConnect:
                    wifi_connect_internal(wifi, &message.connect_message);
                    break;
                case WifiMessageTypeDisconnect:
                    wifi_disconnect_internal_but_keep_active(wifi);
                    break;
            }
        }
    }
}

static void wifi_service_start(Service service) {
    UNUSED(service);
    tt_check(wifi_singleton == NULL);
    wifi_singleton = wifi_alloc();
}

static void wifi_service_stop(Service service) {
    UNUSED(service);
    tt_check(wifi_singleton != NULL);

    WifiRadioState state = wifi_singleton->radio_state;
    if (state != WIFI_RADIO_OFF) {
        wifi_disable(wifi_singleton);
    }

    wifi_free(wifi_singleton);
    wifi_singleton = NULL;

    // wifi_main() cannot be stopped yet as it runs in the main task.
    // We could theoretically exit it, but then we wouldn't be able to restart the service.
    tt_crash("not fully implemented");
}

const ServiceManifest wifi_service = {
    .id = "wifi",
    .on_start = &wifi_service_start,
    .on_stop = &wifi_service_stop
};

#endif
