#include "wifi_manage.h"

#include "app_manifest.h"
#include "furi_core.h"
#include "wifi_manage_view.h"
#include "wifi_manage_state_updating.h"
#include "services/loader/loader.h"

// Forward declarations
static void wifi_manage_event_callback(const void* message, void* context);

static void on_connect(const char* ssid, void* wifi_void) {
    WifiManage* wifi = (WifiManage*)wifi_void;
    wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    // TODO start with params
    loader_start_app("wifi_connect", false);
    FURI_LOG_I("YO", "real connection goes here to %s", ssid);
}

static void on_wifi_toggled(bool enabled) {
    wifi_set_enabled(enabled);
}

static WifiManage* wifi_manage_alloc() {
    WifiManage* wifi = malloc(sizeof(WifiManage));

    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = furi_pubsub_subscribe(wifi_pubsub, &wifi_manage_event_callback, wifi);
    wifi->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    wifi->state = (WifiManageState) {
        .scanning = false,
        .radio_state = wifi_get_enabled() ? WIFI_RADIO_ON : WIFI_RADIO_OFF
    };
    wifi->view = (WifiManageView) {
        .scanning_spinner = NULL
    };
    wifi->bindings = (WifiManageBindings) {
        .on_wifi_toggled = &on_wifi_toggled,
        .on_connect_ssid = &on_connect,
        .on_connect_ssid_context = wifi,
    };

    return wifi;
}

static void wifi_manage_free(WifiManage* wifi) {
    FuriPubSub* wifi_pubsub = wifi_get_pubsub();
    furi_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    furi_mutex_free(wifi->mutex);

    free(wifi);
}

void wifi_manage_lock(WifiManage* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_acquire(wifi->mutex, FuriWaitForever);
}

void wifi_manage_unlock(WifiManage* wifi) {
    furi_assert(wifi);
    furi_assert(wifi->mutex);
    furi_mutex_release(wifi->mutex);
}

static void wifi_manage_event_callback(const void* message, void* context) {
    const WifiEvent* event = (const WifiEvent*)message;
    WifiManage* wifi = (WifiManage*)context;
    switch (event->type) {
        case WifiEventTypeScanStarted:
            wifi_manage_state_set_scanning(wifi, true);
            break;
        case WifiEventTypeScanFinished:
            wifi_manage_state_set_scanning(wifi, false);
            break;
        case WifiEventTypeRadioStateOn:
            wifi_manage_state_set_radio_state(wifi, WIFI_RADIO_ON);
            wifi_scan();
            break;
        case WifiEventTypeRadioStateOnPending:
            wifi_manage_state_set_radio_state(wifi, WIFI_RADIO_ON_PENDING);
            break;
        case WifiEventTypeRadioStateOff:
            wifi_manage_state_set_radio_state(wifi, WIFI_RADIO_OFF);
            break;
        case WifiEventTypeRadioStateOffPending:
            wifi_manage_state_set_radio_state(wifi, WIFI_RADIO_OFF_PENDING);
            break;
    }
}

static void app_show(Context* context, lv_obj_t* parent) {
    WifiManage* wifi = (WifiManage*)context->data;

    wifi_manage_lock(wifi);
    wifi_manage_view_create(&wifi->view, &wifi->bindings, parent);
    wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    wifi_manage_unlock(wifi);

    if (wifi_get_enabled()) {
        wifi_scan();
    }
}

static void app_hide(Context* context) {
    // Nothing to manually free/unsubscribe for views
}

static void app_start(Context* context) {
    WifiManage* wifi = wifi_manage_alloc();
    context->data = wifi;
}

static void app_stop(Context* context) {
    WifiManage* wifi = context->data;
    furi_assert(wifi != NULL);
    wifi_manage_free(wifi);
    context->data = NULL;
}

AppManifest wifi_manage_app = {
    .id = "wifi_manage",
    .name = "Wi-Fi",
    .icon = NULL,
    .type = AppTypeSystem,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show,
    .on_hide = &app_hide
};
