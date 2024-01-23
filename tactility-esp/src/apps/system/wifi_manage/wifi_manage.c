#include "wifi_manage.h"

#include "app.h"
#include "apps/system/wifi_connect/wifi_connect_bundle.h"
#include "services/loader/loader.h"
#include "tactility_core.h"
#include "ui/lvgl_sync.h"
#include "wifi_manage_state_updating.h"
#include "wifi_manage_view.h"
#include "services/wifi/wifi_credentials.h"

#define TAG "wifi_manage"

// Forward declarations
static void wifi_manage_event_callback(const void* message, void* context);

static void on_connect(const char* ssid) {
    char password[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT];
    if (tt_wifi_credentials_get(ssid, password)) {
        TT_LOG_I(TAG, "Connecting with known credentials");
        wifi_connect(ssid, password);
    } else {
        TT_LOG_I(TAG, "Starting connection dialog");
        Bundle bundle = tt_bundle_alloc();
        tt_bundle_put_string(bundle, WIFI_CONNECT_PARAM_SSID, ssid);
        tt_bundle_put_string(bundle, WIFI_CONNECT_PARAM_PASSWORD, "");
        loader_start_app("wifi_connect", false, bundle);
    }
}

static void on_disconnect() {
    wifi_disconnect();
}

static void on_wifi_toggled(bool enabled) {
    wifi_set_enabled(enabled);
}

static WifiManage* wifi_manage_alloc() {
    WifiManage* wifi = malloc(sizeof(WifiManage));

    PubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = tt_pubsub_subscribe(wifi_pubsub, &wifi_manage_event_callback, wifi);
    wifi->mutex = tt_mutex_alloc(MutexTypeNormal);
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
    PubSub* wifi_pubsub = wifi_get_pubsub();
    tt_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    tt_mutex_free(wifi->mutex);

    free(wifi);
}

void wifi_manage_lock(WifiManage* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_acquire(wifi->mutex, TtWaitForever);
}

void wifi_manage_unlock(WifiManage* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_release(wifi->mutex);
}

void wifi_manage_request_view_update(WifiManage* wifi) {
    wifi_manage_lock(wifi);
    if (wifi->view_enabled) {
        if (tt_lvgl_lock(1000)) {
            wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
            tt_lvgl_unlock();
        } else {
            TT_LOG_E(TAG, "failed to lock lvgl");
        }
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

    wifi_manage_request_view_update(wifi);
}

static void app_show(App app, lv_obj_t* parent) {
    WifiManage* wifi = (WifiManage*)tt_app_get_data(app);

    // State update (it has its own locking)
    wifi_manage_state_set_radio_state(wifi, wifi_get_radio_state());
    wifi_manage_state_set_scanning(wifi, wifi_is_scanning());
    wifi_manage_state_update_scanned_records(wifi);

    // View update
    wifi_manage_lock(wifi);
    wifi->view_enabled = true;
    strcpy((char*)wifi->state.connect_ssid, "Connected"); // TODO update with proper SSID
    wifi_manage_view_create(&wifi->view, &wifi->bindings, parent);
    wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    wifi_manage_unlock(wifi);

    WifiRadioState radio_state = wifi_get_radio_state();
    if (radio_state == WIFI_RADIO_ON && !wifi_is_scanning()) {
        wifi_scan();
    }
}

static void app_hide(App app) {
    WifiManage* wifi = (WifiManage*)tt_app_get_data(app);
    wifi_manage_lock(wifi);
    wifi->view_enabled = false;
    wifi_manage_unlock(wifi);
}

static void app_start(App app) {
    WifiManage* wifi = wifi_manage_alloc();
    tt_app_set_data(app, wifi);
}

static void app_stop(App app) {
    WifiManage* wifi = (WifiManage*)tt_app_get_data(app);
    tt_assert(wifi != NULL);
    wifi_manage_free(wifi);
    tt_app_set_data(app, NULL);
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
