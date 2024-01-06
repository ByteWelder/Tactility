#include "furi_core.h"
#include "services/wifi/wifi.h"
#include "app_manifest.h"
#include "mutex.h"
#include "esp_lvgl_port.h"

#include "wifi_state.h"
#include "wifi_view.h"

typedef struct {
    FuriPubSubSubscription* wifi_subscription;
    FuriMutex* mutex;
    WifiState state;
    WifiView view;
} Wifi;

static void wifi_lock(Wifi* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_acquire(wifi->mutex, FuriWaitForever);
}

static void wifi_unlock(Wifi* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_release(wifi->mutex);
}

static void wifi_state_set_scanning(Wifi* wifi, bool is_scanning) {
    wifi_lock(wifi);
    wifi->state.scanning = is_scanning;

    lvgl_port_lock(100);
    wifi_view_update(&wifi->view, &wifi->state);
    lvgl_port_unlock();

    wifi_unlock(wifi);
}

static void wifi_event_callback(const void* message, void* context) {
    const WifiEvent* event = (const WifiEvent*)message;
    Wifi* wifi = (Wifi*)context;
    switch (event->type) {
        case WifiEventTypeScanStarted:
            wifi_state_set_scanning(wifi, true);
            break;
        case WifiEventTypeScanFinished:
            wifi_state_set_scanning(wifi, false);
            break;
    }
}

static void app_show(Context* context, lv_obj_t* parent) {
    Wifi* wifi = (Wifi*)context->data;

    wifi_lock(wifi);
    wifi_view_create(&wifi->view, parent);
    wifi_view_update(&wifi->view, &wifi->state);
    wifi_unlock(wifi);
}

static void app_hide(Context* context) {
    Wifi* wifi = (Wifi*)context->data;

    wifi_lock(wifi);
    wifi_view_clear(&wifi->view);
    wifi_unlock(wifi);
}

static void app_start(Context* context) {
    Wifi* wifi = malloc(sizeof(Wifi));
    context->data = wifi;

    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = furi_pubsub_subscribe(wifi_pubsub, &wifi_event_callback, wifi);
    wifi->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    wifi->state = (WifiState) {
        .scanning = false
    };
    wifi->view = (WifiView) {
        .spinner = NULL
    };

    wifi_scan(); // request scan update
}

static void app_stop(Context* context) {
    Wifi* wifi = context->data;
    furi_assert(wifi != NULL);

    // TODO: Fix potential bug when wifi service is restarted. Listen to wifi service stop event?
    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    furi_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    furi_mutex_free(wifi->mutex);

    context->data = NULL;
    free(wifi);
}

AppManifest wifi_app = {
    .id = "wifi",
    .name = "Wi-Fi",
    .icon = NULL,
    .type = AppTypeSystem,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show,
    .on_hide = &app_hide
};
