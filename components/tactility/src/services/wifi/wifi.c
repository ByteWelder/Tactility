#include "wifi.h"
#include <sys/cdefs.h>

#include "check.h"
#include "esp_wifi.h"
#include "log.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "service_manifest.h"

#define TAG "wifi"

#define SCAN_LIST_SIZE 16

typedef struct {
    FuriMutex* mutex;
    FuriPubSub* pubsub;
    FuriMessageQueue* queue;
} Wifi;

typedef enum {
    WifiMessageTypeRadioOn,
    WifiMessageTypeRadioOff,
    WifiMessageTypeScan
} WifiMessageType;

typedef struct {
    WifiMessageType type;
} WifiMessage;

Wifi* wifi = NULL;

static Wifi* wifi_alloc() {
    Wifi* instance = malloc(sizeof(Wifi));
    instance->mutex = furi_mutex_alloc(FuriMutexTypeRecursive);
    instance->pubsub = furi_pubsub_alloc();
    instance->queue = furi_message_queue_alloc(1, sizeof(WifiMessage));
    return instance;
}

static void wifi_free(Wifi* instance) {
    furi_mutex_free(instance->mutex);
    furi_pubsub_free(instance->pubsub);
    furi_message_queue_free(instance->queue);
    free(instance);
}

FuriPubSub* wifi_get_pubsub() {
    furi_assert(wifi);
    return wifi->pubsub;
}

static void wifi_lock() {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_check(xSemaphoreTakeRecursive(wifi->mutex, portMAX_DELAY) == pdPASS);
}

static void wifi_unlock() {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_check(xSemaphoreGiveRecursive(wifi->mutex) == pdPASS);
}

void wifi_scan() {
    furi_assert(wifi);
    WifiMessage message = { .type = WifiMessageTypeScan };
    // No need to lock for queue
    furi_message_queue_put(wifi->queue, &message, 100 / portTICK_PERIOD_MS);
}

void wifi_set_enabled(bool enabled) {
    if (enabled) {
        WifiMessage message = { .type = WifiMessageTypeRadioOn };
        // No need to lock for queue
        furi_message_queue_put(wifi->queue, &message, 100 / portTICK_PERIOD_MS);
    } else {
        WifiMessage message = { .type = WifiMessageTypeRadioOff };
        // No need to lock for queue
        furi_message_queue_put(wifi->queue, &message, 100 / portTICK_PERIOD_MS);
    }
}

bool wifi_get_enabled() {
    wifi_mode_t mode;
    switch (esp_wifi_get_mode(&mode)) {
        case ESP_OK:
            return mode == WIFI_MODE_STA;
        case ESP_ERR_WIFI_NOT_INIT:
            return false;
        case ESP_ERR_INVALID_ARG:
            return false;
        default:
            furi_crash("unhandled error");
    }
}

static void wifi_enable() {
    if (wifi_get_enabled()) {
        FURI_LOG_W(TAG, "already enabled");
        return;
    }

    WifiEvent turning_on_event = { .type = WifiEventTypeRadioStateOnPending};
    furi_pubsub_publish(wifi->pubsub, &turning_on_event);
    // TODO: send turn off event on failure(s)

//    wifi_lock();
    FURI_LOG_I(TAG, "enabling");

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&cfg) != ESP_OK) {
        FURI_LOG_E(TAG, "wifi init failed");
        return;
    }
    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK) {
        FURI_LOG_E(TAG, "wifi mode setting failed");
        esp_wifi_deinit();
        return;
    }
    if (esp_wifi_start() != ESP_OK) {
        FURI_LOG_E(TAG, "wifi start failed");
        esp_wifi_set_mode(WIFI_MODE_NULL);
        esp_wifi_deinit();
    }

    WifiEvent on_event = { .type = WifiEventTypeRadioStateOn };
    furi_pubsub_publish(wifi->pubsub, &on_event);

    FURI_LOG_I(TAG, "enabled");
//    wifi_unlock();
}

static void wifi_disable() {
    if (!wifi_get_enabled()) {
        FURI_LOG_W(TAG, "already disabled");
        return;
    }

    WifiEvent turning_off_event = { .type = WifiEventTypeRadioStateOffPending};
    furi_pubsub_publish(wifi->pubsub, &turning_off_event);

//    wifi_lock();
    FURI_LOG_I(TAG, "disabling");

    if (esp_wifi_stop() != ESP_OK) {
        FURI_LOG_E(TAG, "failed to stop radio");
        return;
    }

    if (esp_wifi_set_mode(WIFI_MODE_NULL) != ESP_OK) {
        FURI_LOG_E(TAG, "failed to unset mode");
        return;
    }

    if (esp_wifi_deinit() != ESP_OK) {
        FURI_LOG_E(TAG, "failed to deinit");
        return;
    }

    FURI_LOG_I(TAG, "disabled");
//    wifi_unlock();

    WifiEvent turned_off_event = { .type = WifiEventTypeRadioStateOff };
    furi_pubsub_publish(wifi->pubsub, &turned_off_event);
}

static void wifi_scan_internal() {
    if (!wifi_get_enabled()) {
        FURI_LOG_W(TAG, "Cannot scan: wifi not enabled");
        return;
    }

    FURI_LOG_I(TAG, "Starting scan");

    WifiEvent start_event = { .type = WifiEventTypeScanStarted };
    furi_pubsub_publish(wifi->pubsub, &start_event);

    uint16_t number = SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, " - SSID %s (RSSI %d, channel %d)", ap_info[i].ssid, ap_info[i].rssi, ap_info[i].primary);
    }

    WifiEvent finished_event = { .type = WifiEventTypeScanFinished };
    furi_pubsub_publish(wifi->pubsub, &finished_event);
    FURI_LOG_I(TAG, "Finished scan");
}

// ESP wifi APIs need to run from the main task, so we can't just spawn a thread
_Noreturn int32_t wifi_main(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "wifi_main");
    furi_check(wifi != NULL);
    FuriMessageQueue* queue = wifi->queue;

    // TODO: create with "radio on" and destroy with "radio off"?
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    WifiMessage message;
    while (true) {
        if (furi_message_queue_get(queue, &message, FuriWaitForever) == FuriStatusOk) {
            FURI_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case WifiMessageTypeRadioOn:
                    wifi_enable();
                    break;
                case WifiMessageTypeRadioOff:
                    wifi_disable();
                    break;
                case WifiMessageTypeScan:
                    wifi_scan_internal();
                    break;
            }
        }
    }
}

static void wifi_start(Context* context) {
    UNUSED(context);
    furi_check(wifi == NULL);
    wifi = wifi_alloc();
}

static void wifi_stop(Context* context) {
    UNUSED(context);
    furi_check(wifi != NULL);
    // Send stop signal to thread and wait for thread to finish

    if (wifi_get_enabled()) {
        // TODO: disable and wait for it to finish
    }

    wifi_free(wifi);
    wifi = NULL;
}

const ServiceManifest wifi_service = {
    .id = "wifi",
    .on_start = &wifi_start,
    .on_stop = &wifi_stop
};
