#include "wifi_connect.h"

#include "app_manifest.h"
#include "furi_core.h"
#include "wifi_connect_state_updating.h"
#include "esp_lvgl_port.h"

// Forward declarations
static void wifi_connect_event_callback(const void* message, void* context);

static void on_connect(const char* ssid, const char* password, void* wifi_void) {
    WifiConnect* wifi_connect = (WifiConnect*)wifi_void;
    // TODO
    FURI_LOG_I("YO", "real connection goes here to %s, %s", ssid, password);
}

static WifiConnect* wifi_connect_alloc() {
    WifiConnect* wifi = malloc(sizeof(WifiConnect));

    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = furi_pubsub_subscribe(wifi_pubsub, &wifi_connect_event_callback, wifi);
    wifi->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    wifi->state = (WifiConnectState) {
        .radio_state = wifi_get_enabled() ? WIFI_RADIO_ON : WIFI_RADIO_OFF
    };
    wifi->bindings = (WifiConnectBindings) {
        .on_connect_ssid = &on_connect,
        .on_connect_ssid_context = wifi,
    };
    wifi->view_enabled = false;

    return wifi;
}

static void wifi_connect_free(WifiConnect* wifi) {
    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    furi_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    furi_mutex_free(wifi->mutex);

    free(wifi);
}

void wifi_connect_lock(WifiConnect* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_acquire(wifi->mutex, FuriWaitForever);
}

void wifi_connect_unlock(WifiConnect* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_release(wifi->mutex);
}

void wifi_connect_request_view_update(WifiConnect* wifi) {
    wifi_connect_lock(wifi);
    if (wifi->view_enabled) {
        lvgl_port_lock(100);
        wifi_connect_view_update(&wifi->view, &wifi->bindings, &wifi->state);
        lvgl_port_unlock();
    }
    wifi_connect_unlock(wifi);
}

static void wifi_connect_event_callback(const void* message, void* context) {
    const WifiEvent* event = (const WifiEvent*)message;
    WifiConnect* wifi = (WifiConnect*)context;
    switch (event->type) {
        case WifiEventTypeRadioStateOn:
            wifi_connect_state_set_radio_state(wifi, WIFI_RADIO_ON);
            wifi_scan();
            break;
        case WifiEventTypeRadioStateOnPending:
            wifi_connect_state_set_radio_state(wifi, WIFI_RADIO_ON_PENDING);
            break;
        case WifiEventTypeRadioStateOff:
            wifi_connect_state_set_radio_state(wifi, WIFI_RADIO_OFF);
            break;
        case WifiEventTypeRadioStateOffPending:
            wifi_connect_state_set_radio_state(wifi, WIFI_RADIO_OFF_PENDING);
            break;
        default:
            break;
    }
}

static void app_show(Context* context, lv_obj_t* parent) {
    WifiConnect* wifi = (WifiConnect*)context->data;

    wifi_connect_lock(wifi);
    wifi->view_enabled = true;
    wifi_connect_view_create(&wifi->view, &wifi->bindings, parent);
    wifi_connect_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    wifi_connect_unlock(wifi);

    if (wifi_get_enabled()) {
        wifi_scan();
    }
}

static void app_hide(Context* context) {
    WifiConnect * wifi = (WifiConnect *)context->data;
    wifi_connect_lock(wifi);
    wifi->view_enabled = false;
    wifi_connect_unlock(wifi);
}

static void app_start(Context* context) {
    WifiConnect* wifi_connect = wifi_connect_alloc();
    context->data = wifi_connect;
}

static void app_stop(Context* context) {
    WifiConnect* wifi = context->data;
    furi_assert(wifi != NULL);
    wifi_connect_free(wifi);
    context->data = NULL;
}

AppManifest wifi_connect_app = {
    .id = "wifi_connect",
    .name = "Wi-Fi Connect",
    .icon = NULL,
    .type = AppTypeSystem,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show,
    .on_hide = &app_hide
};
