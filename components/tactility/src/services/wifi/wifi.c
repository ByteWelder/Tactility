#include "wifi.h"
#include <sys/cdefs.h>

#include "check.h"
#include "log.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "service_manifest.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


#define TAG "wifi"
#define WIFI_SCAN_RECORD_LIMIT 16 // default, can be overridden

typedef struct {
    /** @brief Locking mechanism for modifying the Wifi instance */
    FuriMutex* mutex;
    /** @brief The public event bus */
    FuriPubSub* pubsub;
    /** @brief The internal message queue */
    FuriMessageQueue* queue;
    /** @brief The network interface when wifi is started */
    esp_netif_t* _Nullable netif;
    /** @brief Scanning results */
    wifi_ap_record_t* _Nullable scan_list;
    /** @brief The current item count in scan_list (-1 when scan_list is NULL) */
    uint16_t scan_list_count;
    /** @brief Maximum amount of records to scan (value > 0) */
    uint16_t scan_list_limit;
    esp_event_handler_instance_t event_handler_any_id;
    esp_event_handler_instance_t event_handler_got_ip;
} WifiConnect;

typedef enum {
    WifiMessageTypeRadioOn,
    WifiMessageTypeRadioOff,
    WifiMessageTypeScan,
    WifiMessageTypeConnect
} WifiMessageType;

typedef struct {
    uint8_t ssid[33];
    uint8_t password[33];
} WifiConnectMessage;

typedef struct {
    WifiMessageType type;
    union {
        WifiConnectMessage connect_message;
    };
} WifiMessage;

static WifiConnect* wifi_singleton = NULL;

// Forward declarations
static void wifi_scan_list_free_safely(WifiConnect* wifi);

// region Alloc

static WifiConnect* wifi_alloc() {
    WifiConnect* instance = malloc(sizeof(WifiConnect));
    instance->mutex = furi_mutex_alloc(FuriMutexTypeRecursive);
    instance->pubsub = furi_pubsub_alloc();
    // TODO: Deal with messages that come in while an action is ongoing
    // for example: when scanning and you turn off the radio, the scan should probably stop or turning off
    // the radio should disable the on/off button in the app as it is pending.
    instance->queue = furi_message_queue_alloc(1, sizeof(WifiMessage));
    instance->netif = NULL;
    instance->scan_list = NULL;
    instance->scan_list_count = 0;
    instance->scan_list_limit = WIFI_SCAN_RECORD_LIMIT;
    instance->event_handler_any_id = NULL;
    instance->event_handler_got_ip = NULL;
    return instance;
}

static void wifi_free(WifiConnect* instance) {
    furi_mutex_free(instance->mutex);
    furi_pubsub_free(instance->pubsub);
    furi_message_queue_free(instance->queue);
    free(instance);
}

// endregion Alloc

// region Public functions

FuriPubSub* wifi_get_pubsub() {
    furi_assert(wifi_singleton);
    return wifi_singleton->pubsub;
}

void wifi_scan() {
    furi_assert(wifi_singleton);
    WifiMessage message = {.type = WifiMessageTypeScan};
    // No need to lock for queue
    furi_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
}

void wifi_connect(const char* ssid, const char* _Nullable password) {
    furi_assert(wifi_singleton);
    WifiMessage message = {
        .type = WifiMessageTypeConnect
    };
    strcpy(message.connect_message.ssid, ssid);
    strcpy(message.connect_message.password, password);
    furi_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
}

void wifi_set_scan_records(uint16_t records) {
    furi_assert(wifi_singleton);
    if (records != wifi_singleton->scan_list_limit) {
        wifi_scan_list_free_safely(wifi_singleton);
        wifi_singleton->scan_list_limit = records;
    }
}

void wifi_get_scan_results(WifiApRecord records[], uint16_t limit, uint16_t* result_count) {
    furi_check(wifi_singleton);
    furi_check(result_count);

    if (wifi_singleton->scan_list_count == 0) {
        *result_count = 0;
    } else {
        uint16_t i = 0;
        FURI_LOG_I(TAG, "processing up to %d APs", wifi_singleton->scan_list_count);
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
        furi_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
    } else {
        WifiMessage message = {.type = WifiMessageTypeRadioOff};
        // No need to lock for queue
        furi_message_queue_put(wifi_singleton->queue, &message, 100 / portTICK_PERIOD_MS);
    }
}

bool wifi_get_enabled() {
    wifi_mode_t mode;
    switch (esp_wifi_get_mode(&mode)) {
        case ESP_OK:
            return mode == WIFI_MODE_STA;
        case ESP_ERR_WIFI_NOT_INIT:
        case ESP_ERR_INVALID_ARG:
            return false;
        default:
            furi_crash("Unhandled error");
    }
}

// endregion Public functions

static void wifi_lock(WifiConnect* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_check(xSemaphoreTakeRecursive(wifi->mutex, portMAX_DELAY) == pdPASS);
}

static void wifi_unlock(WifiConnect* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_check(xSemaphoreGiveRecursive(wifi->mutex) == pdPASS);
}

static void wifi_scan_list_alloc(WifiConnect* wifi) {
    furi_check(wifi->scan_list == NULL);
    wifi->scan_list = malloc(sizeof(wifi_ap_record_t) * wifi->scan_list_limit);
    wifi->scan_list_count = 0;
}

static void wifi_scan_list_alloc_safely(WifiConnect* wifi) {
    if (wifi->scan_list == NULL) {
        wifi_scan_list_alloc(wifi);
    }
}

static void wifi_scan_list_free(WifiConnect* wifi) {
    furi_check(wifi->scan_list != NULL);
    free(wifi->scan_list);
    wifi->scan_list = NULL;
    wifi->scan_list_count = 0;
}

static void wifi_scan_list_free_safely(WifiConnect* wifi) {
    if (wifi->scan_list != NULL) {
        wifi_scan_list_free(wifi);
    }
}

static void wifi_publish_event_simple(WifiConnect* wifi, WifiEventType type) {
    WifiEvent turning_on_event = {.type = type};
    furi_pubsub_publish(wifi->pubsub, &turning_on_event);
}

#define WIFI_CONNECT_RETRY_COUNT 1
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    static EventGroupHandle_t s_wifi_event_group;
    static int s_retry_num = 0;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_CONNECT_RETRY_COUNT) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_enable(WifiConnect* wifi) {
    if (wifi_get_enabled()) {
        FURI_LOG_W(TAG, "Already enabled");
        return;
    }

    FURI_LOG_I(TAG, "Enabling");
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
        FURI_LOG_E(TAG, "Wifi init failed");
        if (init_result == ESP_ERR_NO_MEM) {
            FURI_LOG_E(TAG, "Insufficient memory");
        }
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    // TODO: store in Wifi

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &event_handler,
        NULL,
        &wifi->event_handler_any_id
    ));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &event_handler,
        NULL,
        &wifi->event_handler_got_ip
    ));

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        FURI_LOG_E(TAG, "Wifi mode setting failed");
        esp_wifi_deinit();
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    esp_err_t start_result = esp_wifi_start();
    if (start_result != ESP_OK) {
        FURI_LOG_E(TAG, "Wifi start failed");
        if (start_result == ESP_ERR_NO_MEM) {
            FURI_LOG_E(TAG, "Insufficient memory");
        }
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_wifi_deinit();
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
        return;
    }

    wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOn);
    FURI_LOG_I(TAG, "Enabled");
}

static void wifi_disable(WifiConnect* wifi) {
    if (!wifi_get_enabled()) {
        FURI_LOG_W(TAG, "Already disabled");
        return;
    }

    FURI_LOG_I(TAG, "Disabling");
    wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOffPending);

    // Free up scan list memory
    wifi_scan_list_free_safely(wifi_singleton);

    if (esp_wifi_stop() != ESP_OK) {
        FURI_LOG_E(TAG, "Failed to stop radio");
        wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOn);
        return;
    }

    if (esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK) {
        FURI_LOG_E(TAG, "Failed to unset mode");
    }

    if (esp_event_handler_instance_unregister(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi->event_handler_any_id
    ) != ESP_OK) {
        FURI_LOG_E(TAG, "Failed to unregister id event handler");
    }

    if (esp_event_handler_instance_unregister(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi->event_handler_got_ip
    ) != ESP_OK) {
        FURI_LOG_E(TAG, "Failed to unregister ip event handler");
    }

    if (esp_wifi_deinit() != ESP_OK) {
        FURI_LOG_E(TAG, "Failed to deinit");
    }

    furi_check(wifi->netif != NULL);
    esp_netif_destroy(wifi->netif);
    wifi->netif = NULL;

    wifi_publish_event_simple(wifi, WifiEventTypeRadioStateOff);
    FURI_LOG_I(TAG, "Disabled");
}

static void wifi_scan_internal(WifiConnect* wifi) {
    if (!wifi_get_enabled()) {
        FURI_LOG_W(TAG, "Scan unavailable: wifi not enabled");
        return;
    }

    FURI_LOG_I(TAG, "Starting scan");
    wifi_publish_event_simple(wifi, WifiEventTypeScanStarted);

    // Create scan list if it does not exist
    wifi_scan_list_alloc_safely(wifi);
    wifi->scan_list_count = 0;

    esp_wifi_scan_start(NULL, true);
    uint16_t record_count = wifi->scan_list_limit;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&record_count, wifi->scan_list));
    //    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&wifi->scan_list_count));
    uint16_t safe_record_count = MIN(wifi->scan_list_limit, record_count);
    wifi->scan_list_count = safe_record_count;
    FURI_LOG_I(TAG, "Scanned %u APs. Showing %u:", record_count, safe_record_count);
    for (uint16_t i = 0; i < safe_record_count; i++) {
        wifi_ap_record_t* record = &wifi->scan_list[i];
        FURI_LOG_I(TAG, " - SSID %s (RSSI %d, channel %d)", record->ssid, record->rssi, record->primary);
    }

    esp_wifi_scan_stop();

    wifi_publish_event_simple(wifi, WifiEventTypeScanFinished);
    FURI_LOG_I(TAG, "Finished scan");
}

static void wifi_connect_internal(WifiConnect* wifi, WifiConnectMessage* connect_message) {
}

// ESP wifi APIs need to run from the main task, so we can't just spawn a thread
_Noreturn int32_t wifi_main(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Started main loop");
    furi_check(wifi_singleton != NULL);
    WifiConnect* wifi = wifi_singleton;
    FuriMessageQueue* queue = wifi->queue;

    WifiMessage message;
    while (true) {
        if (furi_message_queue_get(queue, &message, 1000 / portTICK_PERIOD_MS) == FuriStatusOk) {
            FURI_LOG_I(TAG, "Processing message of type %d", message.type);
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
            }
        }
    }
}

static void wifi_service_start(Context* context) {
    UNUSED(context);
    furi_check(wifi_singleton == NULL);
    wifi_singleton = wifi_alloc();
}

static void wifi_service_stop(Context* context) {
    UNUSED(context);
    furi_check(wifi_singleton != NULL);

    if (wifi_get_enabled()) {
        wifi_set_enabled(false);
    }

    wifi_free(wifi_singleton);
    wifi_singleton = NULL;

    // wifi_main() cannot be stopped yet as it runs in the main task.
    // We could theoretically exit it, but then we wouldn't be able to restart the service.
    furi_crash("not fully implemented");
}

const ServiceManifest wifi_service = {
    .id = "wifi",
    .on_start = &wifi_service_start,
    .on_stop = &wifi_service_stop
};
