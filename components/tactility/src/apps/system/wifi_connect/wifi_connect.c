#include "wifi_connect.h"

#include "app.h"
#include "esp_lvgl_port.h"
#include "services/wifi/wifi.h"
#include "tactility_core.h"
#include "wifi_connect_state_updating.h"

#define TAG "wifi_connect"

// Forward declarations
static void wifi_connect_event_callback(const void* message, void* context);

static void on_connect(const char* ssid, const char* password, void* parameter) {
    UNUSED(parameter);
    wifi_connect(ssid, password);
}

static WifiConnect* wifi_connect_alloc() {
    WifiConnect* wifi = malloc(sizeof(WifiConnect));

    PubSub* wifi_pubsub = wifi_get_pubsub();
    wifi->wifi_subscription = tt_pubsub_subscribe(wifi_pubsub, &wifi_connect_event_callback, wifi);
    wifi->mutex = tt_mutex_alloc(MutexTypeNormal);
    wifi->state = (WifiConnectState) {
        .radio_state = wifi_get_radio_state()
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
        if (lvgl_port_lock(1000)) {
            wifi_connect_view_update(&wifi->view, &wifi->bindings, &wifi->state);
            lvgl_port_unlock();
        } else {
            TT_LOG_E(TAG, "failed to lock lvgl");
        }
    }
    wifi_connect_unlock(wifi);
}

static void wifi_connect_event_callback(const void* message, void* context) {
    const WifiEvent* event = (const WifiEvent*)message;
    WifiConnect* wifi = (WifiConnect*)context;
    wifi_connect_state_set_radio_state(wifi, wifi_get_radio_state());
    switch (event->type) {
        case WifiEventTypeRadioStateOn:
            wifi_scan();
            break;
        default:
            break;
    }
    wifi_connect_request_view_update(wifi);
}

static void app_show(App app, lv_obj_t* parent) {
    WifiConnect* wifi = (WifiConnect*)tt_app_get_data(app);

    wifi_connect_lock(wifi);
    wifi->view_enabled = true;
    wifi_connect_view_create(app, wifi, parent);
    wifi_connect_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    wifi_connect_unlock(wifi);
}

static void app_hide(App app) {
    WifiConnect* wifi = (WifiConnect*)tt_app_get_data(app);
    // No need to lock view, as this is called from within Gui's LVGL context
    wifi_connect_view_destroy(&wifi->view);
    wifi_connect_lock(wifi);
    wifi->view_enabled = false;
    wifi_connect_unlock(wifi);
}

static void app_start(App app) {
    WifiConnect* wifi_connect = wifi_connect_alloc(app);
    tt_app_set_data(app, wifi_connect);
}

static void app_stop(App app) {
    WifiConnect* wifi = tt_app_get_data(app);
    tt_assert(wifi != NULL);
    wifi_connect_free(wifi);
    tt_app_set_data(app, NULL);
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
