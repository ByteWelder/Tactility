#include "mutex.h"
#include "check.h"
#include "pubsub.h"
#include "log.h"
#include "message_queue.h"
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
                case WifiMessageTypeServiceStop:
                    exit_requested = true;
                    break;
            }
        }
    }

    return 0;
}

static void wifi_start() {
    furi_check(wifi == NULL);
    wifi = wifi_alloc();
}

static void wifi_stop() {
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
