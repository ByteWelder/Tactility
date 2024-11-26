#include "WifiConnect.h"

#include "app/App.h"
#include "TactilityCore.h"
#include "WifiConnectStateUpdating.h"
#include "service/loader/Loader.h"
#include "service/wifi/Wifi.h"
#include "lvgl/LvglSync.h"

namespace tt::app::wificonnect {

#define TAG "wifi_connect"

// Forward declarations
static void event_callback(const void* message, void* context);

static void on_connect(const service::wifi::settings::WifiApSettings* ap_settings, bool remember, TT_UNUSED void* parameter) {
    auto* wifi = static_cast<WifiConnect*>(parameter);
    state_set_ap_settings(wifi, ap_settings);
    state_set_connecting(wifi, true);
    service::wifi::connect(ap_settings, remember);
}

static WifiConnect* wifi_connect_alloc() {
    auto* wifi = static_cast<WifiConnect*>(malloc(sizeof(WifiConnect)));

    PubSub* wifi_pubsub = service::wifi::get_pubsub();
    wifi->wifi_subscription = tt_pubsub_subscribe(wifi_pubsub, &event_callback, wifi);
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
    PubSub* wifi_pubsub = service::wifi::get_pubsub();
    tt_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    tt_mutex_free(wifi->mutex);

    free(wifi);
}

void lock(WifiConnect* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_acquire(wifi->mutex, TtWaitForever);
}

void unlock(WifiConnect* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_release(wifi->mutex);
}

void request_view_update(WifiConnect* wifi) {
    lock(wifi);
    if (wifi->view_enabled) {
        if (lvgl::lock(1000)) {
            view_update(&wifi->view, &wifi->bindings, &wifi->state);
            lvgl::unlock();
        } else {
            TT_LOG_E(TAG, "Failed to lock lvgl");
        }
    }
    unlock(wifi);
}

static void event_callback(const void* message, void* context) {
    auto* event = static_cast<const service::wifi::WifiEvent*>(message);
    auto* wifi = static_cast<WifiConnect*>(context);
    switch (event->type) {
        case service::wifi::WifiEventTypeConnectionFailed:
            if (wifi->state.is_connecting) {
                state_set_connecting(wifi, false);
                state_set_radio_error(wifi, true);
                request_view_update(wifi);
            }
            break;
        case service::wifi::WifiEventTypeConnectionSuccess:
            if (wifi->state.is_connecting) {
                state_set_connecting(wifi, false);
                service::loader::stop_app();
            }
            break;
        default:
            break;
    }
    request_view_update(wifi);
}

static void app_show(App app, lv_obj_t* parent) {
    auto* wifi = static_cast<WifiConnect*>(tt_app_get_data(app));

    lock(wifi);
    wifi->view_enabled = true;
    view_create(app, wifi, parent);
    view_update(&wifi->view, &wifi->bindings, &wifi->state);
    unlock(wifi);
}

static void app_hide(App app) {
    auto* wifi = static_cast<WifiConnect*>(tt_app_get_data(app));
    // No need to lock view, as this is called from within Gui's LVGL context
    view_destroy(&wifi->view);
    lock(wifi);
    wifi->view_enabled = false;
    unlock(wifi);
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

extern const Manifest manifest = {
    .id = "WifiConnect",
    .name = "Wi-Fi Connect",
    .icon = LV_SYMBOL_WIFI,
    .type = TypeSettings,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show,
    .on_hide = &app_hide
};

} // namespace
