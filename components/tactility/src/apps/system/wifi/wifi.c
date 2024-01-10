#include "wifi.h"

#include "app_manifest.h"
#include "furi_core.h"
#include "wifi_main_view.h"
#include "wifi_state_updating.h"

// Forward declarations
static void wifi_event_callback(const void* message, void* context);

static void on_wifi_show_connect_dialog(const char* ssid, void* wifi_void) {
    Wifi* wifi = (Wifi*)wifi_void;
    memcpy(wifi->state.connect_ssid, ssid, 33);
    wifi->state.active_screen = WIFI_SCREEN_CONNECT;
    wifi_view_update(&wifi->view, &wifi->bindings, &wifi->state);
}

static void on_wifi_hide_connect_dialog(void* wifi_void) {
    Wifi* wifi = (Wifi*)wifi_void;
    wifi->state.active_screen = WIFI_SCREEN_MAIN;
    wifi_view_update(&wifi->view, &wifi->bindings, &wifi->state);
}

static void on_connect(const char* ssid, const char* password, void* wifi_void) {
    Wifi* wifi = (Wifi*)wifi_void;
    wifi->state.active_screen = WIFI_SCREEN_MAIN;
    wifi_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    FURI_LOG_I("YO", "real connection goes here to %s, %s", ssid, password);
}

static void on_wifi_toggled(bool enabled) {
    wifi_set_enabled(enabled);
}

static Wifi* wifi_alloc() {
    Wifi* wifi = malloc(sizeof(Wifi));

    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = furi_pubsub_subscribe(wifi_pubsub, &wifi_event_callback, wifi);
    wifi->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    wifi->state = (WifiState) {
        .active_screen = WIFI_SCREEN_MAIN,
        .scanning = false,
        .radio_state = wifi_get_enabled() ? WIFI_RADIO_ON : WIFI_RADIO_OFF
    };
    wifi->view.main_view = (WifiMainView) {
        .scanning_spinner = NULL
    };
    wifi->bindings = (WifiBindings) {
        .on_wifi_toggled = &on_wifi_toggled,
        .on_show_connect_dialog = &on_wifi_show_connect_dialog,
        .on_show_connect_dialog_context = wifi,
        .on_hide_connect_dialog = &on_wifi_hide_connect_dialog,
        .on_hide_connect_dialog_context = wifi,
        .on_connect_ssid = &on_connect,
        .on_connect_ssid_context = wifi,
    };

    return wifi;
}

static void wifi_free(Wifi* wifi) {
    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    furi_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    furi_mutex_free(wifi->mutex);

    free(wifi);
}

void wifi_lock(Wifi* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_acquire(wifi->mutex, FuriWaitForever);
}

void wifi_unlock(Wifi* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_release(wifi->mutex);
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
        case WifiEventTypeRadioStateOn:
            wifi_state_set_radio_state(wifi, WIFI_RADIO_ON);
            wifi_scan();
            break;
        case WifiEventTypeRadioStateOnPending:
            wifi_state_set_radio_state(wifi, WIFI_RADIO_ON_PENDING);
            break;
        case WifiEventTypeRadioStateOff:
            wifi_state_set_radio_state(wifi, WIFI_RADIO_OFF);
            break;
        case WifiEventTypeRadioStateOffPending:
            wifi_state_set_radio_state(wifi, WIFI_RADIO_OFF_PENDING);
            break;
    }
}

static void app_show(Context* context, lv_obj_t* parent) {
    Wifi* wifi = (Wifi*)context->data;

    wifi_lock(wifi);
    wifi_view_create(&wifi->view, &wifi->bindings, parent);
    wifi_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    wifi_unlock(wifi);

    if (wifi_get_enabled()) {
        wifi_scan();
    }
}

static void app_hide(Context* context) {
    // Nothing to manually free/unsubscribe for views
}

static void app_start(Context* context) {
    Wifi* wifi = wifi_alloc();
    context->data = wifi;
}

static void app_stop(Context* context) {
    Wifi* wifi = context->data;
    furi_assert(wifi != NULL);
    wifi_free(wifi);
    context->data = NULL;
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
