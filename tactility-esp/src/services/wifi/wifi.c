#include "wifi.h"

#include "check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "log.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "service.h"
#include "ui/statusbar.h"
#include <sys/cdefs.h>

#define TAG "wifi"
#define WIFI_SCAN_RECORD_LIMIT 16 // default, can be overridden
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

typedef struct {
    /** @brief Locking mechanism for modifying the Wifi instance */
    Mutex mutex;
    /** @brief The public event bus */
    PubSub* pubsub;
    /** @brief The internal message queue */
    MessageQueue* queue;
    /** @brief The network interface when wifi is started */
    esp_netif_t* _Nullable netif;
    /** @brief Scanning results */
    wifi_ap_record_t* _Nullable scan_list;
    /** @brief The current item count in scan_list (-1 when scan_list is NULL) */
    uint16_t scan_list_count;
    /** @brief Maximum amount of records to scan (value > 0) */
    uint16_t scan_list_limit;
    int8_t statusbar_icon_id;
    bool scan_active;
    esp_event_handler_instance_t event_handler_any_id;
    esp_event_handler_instance_t event_handler_got_ip;
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
    instance->netif = NULL;
    instance->scan_active = false;
    instance->scan_list = NULL;
    instance->scan_list_count = 0;
    instance->scan_list_limit = WIFI_SCAN_RECORD_LIMIT;
    instance->event_handler_any_id = NULL;
    instance->event_handler_got_ip = NULL;
    instance->event_group = xEventGroupCreate();
    instance->radio_state = WIFI_RADIO_OFF;
    instance->statusbar_icon_id = tt_statusbar_icon_add("A:/assets/ic_small_wifi_off.png");
    return instance;
}

static void wifi_free(Wifi* instance) {
    tt_statusbar_icon_remove(instance->statusbar_icon_id);
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
        uint16_t last_index = TT_MIN(wifi_singleton->scan_list_count, limit);
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
    switch (type) {
        case WifiEventTypeRadioStateOn:
            break;
        case WifiEventTypeRadioStateOnPending:
            break;
        case WifiEventTypeRadioStateOff:
            tt_statusbar_icon_set_image(wifi->statusbar_icon_id, "A:/assets/ic_small_wifi_off.png");
            break;
        case WifiEventTypeRadioStateOffPending:
            break;
        case WifiEventTypeScanStarted:
            break;
        case WifiEventTypeScanFinished:
            break;
        case WifiEventTypeDisconnected:
            tt_statusbar_icon_set_image(wifi->statusbar_icon_id, "A:/assets/ic_small_wifi_off.png");
            break;
        case WifiEventTypeConnectionPending:
            tt_statusbar_icon_set_image(wifi->statusbar_icon_id, "A:/assets/network_wifi_1_bar.png");
            break;
        case WifiEventTypeConnectionSuccess:
            // TODO: update with actual bars
            tt_statusbar_icon_set_image(wifi->statusbar_icon_id, "A:/assets/network_wifi.png");
            break;
        case WifiEventTypeConnectionFailed:
            tt_statusbar_icon_set_image(wifi->statusbar_icon_id, "A:/assets/ic_small_wifi_off.png");
            break;
    }

    tt_pubsub_publish(wifi->pubsub, &turning_on_event);
}

static void event_handler(TT_UNUSED void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        TT_LOG_I(TAG, "event_handler: sta start");
        if (wifi_singleton->radio_state == WIFI_RADIO_CONNECTION_PENDING) {
            esp_wifi_connect();
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupSetBits(wifi_singleton->event_group, WIFI_FAIL_BIT);
        TT_LOG_I(TAG, "event_handler: disconnected");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        TT_LOG_I(TAG, "event_handler: got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_singleton->event_group, WIFI_CONNECTED_BIT);
    }
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

    TT_LOG_I(TAG, "Enabling");
    wifi->radio_state = WIFI_RADIO_ON_PENDING;
    wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOnPending);

    if (wifi->netif != NULL) {
        esp_netif_destroy(wifi->netif);
    }
    wifi->netif = esp_netif_create_default_wifi_sta();

    // Warning: this is the memory-intensive operation
    // It uses over 117kB of RAM with default settings for S3 on IDF v5.1.2
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t init_result = esp_wifi_init(&config);
    if (init_result != ESP_OK) {
        TT_LOG_E(TAG, "Wifi init failed");
        if (init_result == ESP_ERR_NO_MEM) {
            TT_LOG_E(TAG, "Insufficient memory");
        }
        wifi->radio_state = WIFI_RADIO_OFF;
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    esp_wifi_set_storage(WIFI_STORAGE_RAM);

    // TODO: don't crash on check failure
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL,
        &wifi->event_handler_any_id
    ));

    // TODO: don't crash on check failure
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL,
        &wifi->event_handler_got_ip
    ));

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        TT_LOG_E(TAG, "Wifi mode setting failed");
        wifi->radio_state = WIFI_RADIO_OFF;
        esp_wifi_deinit();
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    esp_err_t start_result = esp_wifi_start();
    if (start_result != ESP_OK) {
        TT_LOG_E(TAG, "Wifi start failed");
        if (start_result == ESP_ERR_NO_MEM) {
            TT_LOG_E(TAG, "Insufficient memory");
        }
        wifi->radio_state = WIFI_RADIO_OFF;
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_wifi_deinit();
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
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

    TT_LOG_I(TAG, "Disabling");
    wifi->radio_state = WIFI_RADIO_OFF_PENDING;
    wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOffPending);

    // Free up scan list memory
    wifi_scan_list_free_safely(wifi_singleton);

    if (esp_wifi_stop() != ESP_OK) {
        TT_LOG_E(TAG, "Failed to stop radio");
        wifi->radio_state = WIFI_RADIO_ON;
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOn);
        return;
    }

    if (esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to unset mode");
    }

    if (esp_event_handler_instance_unregister(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            wifi->event_handler_any_id
        ) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to unregister id event handler");
    }

    if (esp_event_handler_instance_unregister(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            wifi->event_handler_got_ip
        ) != ESP_OK) {
        TT_LOG_E(TAG, "Failed to unregister ip event handler");
    }

    if (esp_wifi_deinit() != ESP_OK) {
        TT_LOG_E(TAG, "Failed to deinit");
    }

    tt_check(wifi->netif != NULL);
    esp_netif_destroy(wifi->netif);
    wifi->netif = NULL;

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

    // Create scan list if it does not exist
    wifi_scan_list_alloc_safely(wifi);
    wifi->scan_list_count = 0;

    esp_wifi_scan_start(NULL, true);
    uint16_t record_count = wifi->scan_list_limit;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_count, wifi->scan_list));
    uint16_t safe_record_count = TT_MIN(wifi->scan_list_limit, record_count);
    wifi->scan_list_count = safe_record_count;
    TT_LOG_I(TAG, "Scanned %u APs. Showing %u:", record_count, safe_record_count);
    for (uint16_t i = 0; i < safe_record_count; i++) {
        wifi_ap_record_t* record = &wifi->scan_list[i];
        TT_LOG_I(TAG, " - SSID %s (RSSI %d, channel %d)", record->ssid, record->rssi, record->primary);
    }

    esp_wifi_scan_stop();

    wifi_publish_event_simple(wifi, WifiEventTypeScanFinished);
    wifi->scan_active = false;
    TT_LOG_I(TAG, "Finished scan");
}

static void wifi_connect_internal(Wifi* wifi, WifiConnectMessage* connect_message) {
    // TODO: only when connected!
    wifi_disconnect_internal(wifi);

    wifi->radio_state = WIFI_RADIO_CONNECTION_PENDING;

    wifi_publish_event_simple(wifi, WifiEventTypeConnectionPending);

    wifi_config_t wifi_config = {
        .sta = {
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = WIFI_AUTH_WPA2_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .sae_h2e_identifier = {0},
        },
    };
    memcpy(wifi_config.sta.ssid, connect_message->ssid, 32);
    memcpy(wifi_config.sta.password, connect_message->password, 64);

    esp_err_t set_config_result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (set_config_result != ESP_OK) {
        wifi->radio_state = WIFI_RADIO_ON;
        TT_LOG_E(TAG, "failed to set wifi config (%s)", esp_err_to_name(set_config_result));
        wifi_publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        return;
    }

    esp_err_t wifi_start_result = esp_wifi_start();
    if (wifi_start_result != ESP_OK) {
        wifi->radio_state = WIFI_RADIO_ON;
        TT_LOG_E(TAG, "failed to start wifi to begin connecting (%s)", esp_err_to_name(wifi_start_result));
        wifi_publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        return;
    }

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
     * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
     * The bits are set by wifi_event_handler() */
    EventBits_t bits = xEventGroupWaitBits(
        wifi->event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        wifi->radio_state = WIFI_RADIO_CONNECTION_ACTIVE;
        wifi_publish_event_simple(wifi, WifiEventTypeConnectionSuccess);
        TT_LOG_I(TAG, "Connected to %s", connect_message->ssid);
    } else if (bits & WIFI_FAIL_BIT) {
        wifi->radio_state = WIFI_RADIO_ON;
        wifi_publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        TT_LOG_I(TAG, "Failed to connect to %s", connect_message->ssid);
    } else {
        wifi->radio_state = WIFI_RADIO_ON;
        wifi_publish_event_simple(wifi, WifiEventTypeConnectionFailed);
        TT_LOG_E(TAG, "UNEXPECTED EVENT");
    }
}

static void wifi_disconnect_internal(Wifi* wifi) {
    esp_err_t stop_result = esp_wifi_stop();
    if (stop_result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to disconnect (%s)", esp_err_to_name(stop_result));
    } else {
        wifi->radio_state = WIFI_RADIO_ON;
        wifi_publish_event_simple(wifi, WifiEventTypeDisconnected);
        TT_LOG_I(TAG, "Disconnected");
    }
}

static void wifi_disconnect_internal_but_keep_active(Wifi* wifi) {
    esp_err_t stop_result = esp_wifi_stop();
    if (stop_result != ESP_OK) {
        TT_LOG_E(TAG, "Failed to disconnect (%s)", esp_err_to_name(stop_result));
        return;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = {0},
            .password = {0},
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_UNSPECIFIED,
            .sae_h2e_identifier = {0},
        },
    };

    esp_err_t set_config_result = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (set_config_result != ESP_OK) {
        // TODO: disable radio, because radio state is in limbo between off and on
        wifi->radio_state = WIFI_RADIO_OFF;
        TT_LOG_E(TAG, "failed to set wifi config (%s)", esp_err_to_name(set_config_result));
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    esp_err_t wifi_start_result = esp_wifi_start();
    if (wifi_start_result != ESP_OK) {
        // TODO: disable radio, because radio state is in limbo between off and on
        wifi->radio_state = WIFI_RADIO_OFF;
        TT_LOG_E(TAG, "failed to start wifi to begin connecting (%s)", esp_err_to_name(wifi_start_result));
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    wifi->radio_state = WIFI_RADIO_ON;
    wifi_publish_event_simple(wifi, WifiEventTypeDisconnected);
    TT_LOG_I(TAG, "Disconnected");
}

// ESP wifi APIs need to run from the main task, so we can't just spawn a thread
_Noreturn int32_t wifi_main(TT_UNUSED void* parameter) {
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

static void wifi_service_start(TT_UNUSED Service service) {
    tt_check(wifi_singleton == NULL);
    wifi_singleton = wifi_alloc();
}

static void wifi_service_stop(TT_UNUSED Service service) {
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

