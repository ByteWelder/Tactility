#include "wifi_connect.h"

#include "app.h"
#include "services/loader/loader.h"
#include "services/wifi/wifi.h"
#include "tactility_core.h"
#include "ui/lvgl_sync.h"
#include "wifi_connect_state_updating.h"

#define TAG "wifi_connect"

// Forward declarations
static void wifi_connect_event_callback(const void* message, void* context);

static void on_connect(const WifiApSettings* ap_settings, bool remember, TT_UNUSED void* parameter) {
    auto* wifi = static_cast<WifiConnect*>(parameter);
    wifi_connect_state_set_ap_settings(wifi, ap_settings);
    wifi_connect_state_set_connecting(wifi, true);
    wifi_connect(ap_settings, remember);
}

static WifiConnect* wifi_connect_alloc() {
    auto* wifi = static_cast<WifiConnect*>(malloc(sizeof(WifiConnect)));

    PubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = tt_pubsub_subscribe(wifi_pubsub, &wifi_connect_event_callback, wifi);
    wifi->mutex = tt_mutex_alloc(MutexTypeNormal);
    wifi->state = (WifiConnectState) {
        .settings = {
            .ssid = { 0 },
            .password = { 0 },
            .auto_connect = false,
        },
        .connection_error = false,
        .is_connecting = false
    };
    wifi->bindings = (WifiConnectBindings) {
        .on_connect_ssid = &on_connect,
        .on_connect_ssid_context = wifi,
    };
    wifi->view_enabled = false;

    return wifi;
}

static void wifi_connect_free(WifiConnect* wifi) {
    PubSub* wifi_pubsub = wifi_get_pubsub();
    tt_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    tt_mutex_free(wifi->mutex);

    free(wifi);
}

void wifi_connect_lock(WifiConnect* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_acquire(wifi->mutex, TtWaitForever);
}

void wifi_connect_unlock(WifiConnect* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_release(wifi->mutex);
}

void wifi_connect_request_view_update(WifiConnect* wifi) {
    wifi_connect_lock(wifi);
    if (wifi->view_enabled) {
        if (tt_lvgl_lock(1000)) {
            wifi_connect_view_update(&wifi->view, &wifi->bindings, &wifi->state);
            tt_lvgl_unlock();
        } else {
            TT_LOG_E(TAG, "Failed to lock lvgl");
        }
    }
    wifi_connect_unlock(wifi);
}

static void wifi_connect_event_callback(const void* message, void* context) {
    auto* event = static_cast<const WifiEvent*>(message);
    auto* wifi = static_cast<WifiConnect*>(context);
    switch (event->type) {
        case WifiEventTypeConnectionFailed:
            if (wifi->state.is_connecting) {
                wifi_connect_state_set_connecting(wifi, false);
                wifi_connect_state_set_radio_error(wifi, true);
                wifi_connect_request_view_update(wifi);
            }
            break;
        case WifiEventTypeConnectionSuccess:
            if (wifi->state.is_connecting) {
                wifi_connect_state_set_connecting(wifi, false);
                loader_stop_app();
            }
            break;
        default:
            break;
    }
    wifi_connect_request_view_update(wifi);
}

static void app_show(App app, lv_obj_t* parent) {
    auto* wifi = static_cast<WifiConnect*>(tt_app_get_data(app));

    wifi_connect_lock(wifi);
    wifi->view_enabled = true;
    wifi_connect_view_create(app, wifi, parent);
    wifi_connect_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    wifi_connect_unlock(wifi);
}

static void app_hide(App app) {
    auto* wifi = static_cast<WifiConnect*>(tt_app_get_data(app));
    // No need to lock view, as this is called from within Gui's LVGL context
    wifi_connect_view_destroy(&wifi->view);
    wifi_connect_lock(wifi);
    wifi->view_enabled = false;
    wifi_connect_unlock(wifi);
}

static void app_start(App app) {
    auto* wifi_connect = wifi_connect_alloc();
    tt_app_set_data(app, wifi_connect);
}

static void app_stop(App app) {
    auto* wifi = static_cast<WifiConnect*>(tt_app_get_data(app));
    tt_assert(wifi != nullptr);
    wifi_connect_free(wifi);
    tt_app_set_data(app, nullptr);
}

extern const AppManifest wifi_connect_app = {
    .id = "wifi_connect",
    .name = "Wi-Fi Connect",
    .icon = LV_SYMBOL_WIFI,
    .type = AppTypeSettings,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show,
    .on_hide = &app_hide
};
