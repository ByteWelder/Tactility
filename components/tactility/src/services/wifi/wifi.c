#include "wifi.h"

#include "check.h"
#include "esp_wifi.h"
#include "log.h"
#include "message_queue.h"
#include "mutex.h"
#include "pubsub.h"
#include "service_manifest.h"

#define TAG "wifi"

// Forward declarations
static int32_t wifi_main(void* p);

typedef struct {
    FuriMutex* mutex;
    FuriThread* thread;
    FuriPubSub* pubsub;
    FuriMessageQueue* queue;
} Wifi;

typedef enum {
    WifiMessageTypeRadioOn,
    WifiMessageTypeRadioOff,
    WifiMessageTypeScan,
    WifiMessageTypeServiceStop,
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
    instance->thread = furi_thread_alloc_ex(
        "wifi",
        2048, // TODO: Update relevant value after testing
        &wifi_main,
        NULL
    );
    return instance;
}

static void wifi_free(Wifi* instance) {
    furi_mutex_free(instance->mutex);
    furi_pubsub_free(instance->pubsub);
    furi_message_queue_free(instance->queue);
    furi_thread_free(instance->thread);
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

#define SCAN_LIST_SIZE 16

static void wifi_scan_internal() {
    WifiEvent start_event = { .type = WifiEventTypeScanStarted };

    furi_pubsub_publish(wifi->pubsub, &start_event);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t* sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    uint16_t number = SCAN_LIST_SIZE;
    wifi_ap_record_t ap_info[SCAN_LIST_SIZE];
    uint16_t ap_count = 0;
    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_wifi_scan_start(NULL, true);
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
    ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
    for (int i = 0; (i < SCAN_LIST_SIZE) && (i < ap_count); i++) {
        ESP_LOGI(TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(TAG, "RSSI \t\t%d", ap_info[i].rssi);
//        print_auth_mode(ap_info[i].authmode);
//        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
//            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
//        }
        ESP_LOGI(TAG, "Channel \t\t%d\n", ap_info[i].primary);
    }

    WifiEvent finished_event = { .type = WifiEventTypeScanFinished };
    furi_pubsub_publish(wifi->pubsub, &finished_event);
}

static int32_t wifi_main(void* p) {
    UNUSED(p);

    furi_check(wifi != NULL);
    FuriMessageQueue* queue = wifi->queue;

    WifiMessage message;
    bool exit_requested = false;
    while (!exit_requested) {
        if (furi_message_queue_get(queue, &message, FuriWaitForever) == FuriStatusOk) {
            FURI_LOG_I(TAG, "Processing message of type %d", message.type);
            switch (message.type) {
                case WifiMessageTypeRadioOn:
                    break;
                case WifiMessageTypeRadioOff:
                    break;
                case WifiMessageTypeScan:
                    wifi_scan_internal();
                    break;
                case WifiMessageTypeServiceStop:
                    exit_requested = true;
                    break;
            }
        }
    }

    return 0;
}

static void wifi_start(Context* context) {
    UNUSED(context);
    furi_check(wifi == NULL);
    wifi = wifi_alloc();
}

static void wifi_stop(Context* context) {
    UNUSED(context);
    furi_check(wifi != NULL);
    WifiMessage message = {
        .type = WifiMessageTypeServiceStop
    };
    furi_message_queue_put(wifi->queue, &message, FuriWaitForever);
    furi_thread_join(wifi->thread);

    wifi_free(wifi);
    wifi = NULL;
}

const ServiceManifest wifi_service = {
    .id = "wifi",
    .on_start = &wifi_start,
    .on_stop = &wifi_stop
};
