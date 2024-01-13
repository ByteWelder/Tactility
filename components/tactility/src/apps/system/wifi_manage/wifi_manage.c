#include "wifi_manage.h"

#include "app.h"
#include "apps/system/wifi_connect/wifi_connect_bundle.h"
#include "esp_lvgl_port.h"
#include "furi_core.h"
#include "services/loader/loader.h"
#include "wifi_manage_state_updating.h"
#include "wifi_manage_view.h"

// Forward declarations
static void wifi_manage_event_callback(const void* message, void* context);

static void on_connect(const char* ssid) {
    Bundle bundle = bundle_alloc();
    bundle_put_string(bundle, WIFI_CONNECT_PARAM_SSID, ssid);
    bundle_put_string(bundle, WIFI_CONNECT_PARAM_PASSWORD, ""); // TODO: Implement from cache
    loader_start_app("wifi_connect", false, bundle);
}

static void on_disconnect() {
    wifi_disconnect();
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
        .scanning = wifi_is_scanning(),
        .radio_state = wifi_get_radio_state()
    };
    wifi->view_enabled = false;
    wifi->bindings = (WifiManageBindings) {
        .on_wifi_toggled = &on_wifi_toggled,
        .on_connect_ssid = &on_connect,
        .on_disconnect = &on_disconnect
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

void wifi_manage_request_view_update(WifiManage* wifi) {
    wifi_manage_lock(wifi);
    if (wifi->view_enabled) {
        lvgl_port_lock(100);
        wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
        lvgl_port_unlock();
    }
    wifi_manage_unlock(wifi);
}

static void wifi_manage_event_callback(const void* message, void* context) {
    const WifiEvent* event = (const WifiEvent*)message;
    WifiManage* wifi = (WifiManage*)context;
    wifi_manage_state_set_radio_state(wifi, wifi_get_radio_state());
    switch (event->type) {
        case WifiEventTypeScanStarted:
            wifi_manage_state_set_scanning(wifi, true);
            break;
        case WifiEventTypeScanFinished:
            wifi_manage_state_set_scanning(wifi, false);
            wifi_manage_state_update_scanned_records(wifi);
            break;
        case WifiEventTypeRadioStateOn:
            if (!wifi_is_scanning()) {
                wifi_scan();
            }
            break;
        default:
            break;
    }
}

static void app_show(App app, lv_obj_t* parent) {
    WifiManage* wifi = (WifiManage*)app_get_data(app);

    // State update (it has its own locking)
    wifi_manage_state_set_radio_state(wifi, wifi_get_radio_state());
    wifi_manage_state_set_scanning(wifi, wifi_is_scanning());
    wifi_manage_state_update_scanned_records(wifi);

    // View update
    wifi_manage_lock(wifi);
    wifi->view_enabled = true;
    wifi_manage_view_create(&wifi->view, &wifi->bindings, parent);
    wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    wifi_manage_unlock(wifi);

    WifiRadioState radio_state = wifi_get_radio_state();
    if (radio_state == WIFI_RADIO_ON && !wifi_is_scanning()) {
        wifi_scan();
    }
}

static void app_hide(App app) {
    WifiManage* wifi = (WifiManage*)app_get_data(app);
    wifi_manage_lock(wifi);
    wifi->view_enabled = false;
    wifi_manage_unlock(wifi);
}

static void app_start(App app) {
    WifiManage* wifi = wifi_manage_alloc();
    app_set_data(app, wifi);
}

static void app_stop(App app) {
    WifiManage* wifi = (WifiManage*)app_get_data(app);
    furi_assert(wifi != NULL);
    wifi_manage_free(wifi);
    app_set_data(app, NULL);
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
