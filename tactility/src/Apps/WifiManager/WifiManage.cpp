#include "WifiManage.h"

#include "App.h"
#include "Apps/WifiConnect/WifiConnectBundle.h"
#include "TactilityCore.h"
#include "Services/Loader/Loader.h"
#include "Services/Wifi/WifiSettings.h"
#include "Ui/LvglSync.h"
#include "WifiManageStateUpdating.h"
#include "WifiManageView.h"

namespace tt::app::wifi_manage {

#define TAG "wifi_manage"

// Forward declarations
static void event_callback(const void* message, void* context);

static void on_connect(const char* ssid) {
    service::wifi::settings::WifiApSettings settings;
    if (service::wifi::settings::load(ssid, &settings)) {
        TT_LOG_I(TAG, "Connecting with known credentials");
        service::wifi::connect(&settings, false);
    } else {
        TT_LOG_I(TAG, "Starting connection dialog");
        Bundle bundle;
        bundle.putString(WIFI_CONNECT_PARAM_SSID, ssid);
        bundle.putString(WIFI_CONNECT_PARAM_PASSWORD, "");
        service::loader::start_app("wifi_connect", false, bundle);
    }
}

static void on_disconnect() {
    service::wifi::disconnect();
}

static void on_wifi_toggled(bool enabled) {
    service::wifi::set_enabled(enabled);
}

static WifiManage* wifi_manage_alloc() {
    auto* wifi = static_cast<WifiManage*>(malloc(sizeof(WifiManage)));

    wifi->wifi_subscription = nullptr;
    wifi->mutex = tt_mutex_alloc(MutexTypeNormal);
    wifi->state = (WifiManageState) {
        .scanning = service::wifi::is_scanning(),
        .radio_state = service::wifi::get_radio_state(),
        .connect_ssid = { 0 },
        .ap_records = { },
        .ap_records_count = 0
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
    tt_mutex_free(wifi->mutex);

    free(wifi);
}

void lock(WifiManage* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_acquire(wifi->mutex, TtWaitForever);
}

void unlock(WifiManage* wifi) {
    tt_assert(wifi);
    tt_assert(wifi->mutex);
    tt_mutex_release(wifi->mutex);
}

void request_view_update(WifiManage* wifi) {
    lock(wifi);
    if (wifi->view_enabled) {
        if (lvgl::lock(1000)) {
            view_update(&wifi->view, &wifi->bindings, &wifi->state);
            lvgl::unlock();
        } else {
            TT_LOG_E(TAG, "failed to lock lvgl");
        }
    }
    unlock(wifi);
}

static void wifi_manage_event_callback(const void* message, void* context) {
    auto* event = (service::wifi::WifiEvent*)message;
    auto* wifi = (WifiManage*)context;
    TT_LOG_I(TAG, "Update with state %d", service::wifi::get_radio_state());
    state_set_radio_state(wifi, service::wifi::get_radio_state());
    switch (event->type) {
        case tt::service::wifi::WifiEventTypeScanStarted:
            state_set_scanning(wifi, true);
            break;
        case tt::service::wifi::WifiEventTypeScanFinished:
            state_set_scanning(wifi, false);
            state_update_scanned_records(wifi);
            break;
        case tt::service::wifi::WifiEventTypeRadioStateOn:
            if (!service::wifi::is_scanning()) {
                service::wifi::scan();
            }
            break;
        default:
            break;
    }

    request_view_update(wifi);
}

static void app_show(App app, lv_obj_t* parent) {
    auto* wifi = (WifiManage*)tt_app_get_data(app);

    PubSub* wifi_pubsub = service::wifi::get_pubsub();
    wifi->wifi_subscription = tt_pubsub_subscribe(wifi_pubsub, &wifi_manage_event_callback, wifi);

    // State update (it has its own locking)
    state_set_radio_state(wifi, service::wifi::get_radio_state());
    state_set_scanning(wifi, service::wifi::is_scanning());
    state_update_scanned_records(wifi);

    // View update
    lock(wifi);
    wifi->view_enabled = true;
    strcpy((char*)wifi->state.connect_ssid, "Connected"); // TODO update with proper SSID
    view_create(app, &wifi->view, &wifi->bindings, parent);
    view_update(&wifi->view, &wifi->bindings, &wifi->state);
    unlock(wifi);

    service::wifi::WifiRadioState radio_state = service::wifi::get_radio_state();
    bool can_scan = radio_state == service::wifi::WIFI_RADIO_ON ||
        radio_state == service::wifi::WIFI_RADIO_CONNECTION_PENDING ||
        radio_state == service::wifi::WIFI_RADIO_CONNECTION_ACTIVE;
    if (can_scan && !service::wifi::is_scanning()) {
        service::wifi::scan();
    }
}

static void app_hide(App app) {
    auto* wifi = (WifiManage*)tt_app_get_data(app);
    lock(wifi);
    PubSub* wifi_pubsub = service::wifi::get_pubsub();
    tt_pubsub_unsubscribe(wifi_pubsub, wifi->wifi_subscription);
    wifi->wifi_subscription = nullptr;
    wifi->view_enabled = false;
    unlock(wifi);
}

static void app_start(App app) {
    WifiManage* wifi = wifi_manage_alloc();
    tt_app_set_data(app, wifi);
}

static void app_stop(App app) {
    auto* wifi = (WifiManage*)tt_app_get_data(app);
    tt_assert(wifi != nullptr);
    wifi_manage_free(wifi);
    tt_app_set_data(app, nullptr);
}

extern const AppManifest manifest = {
    .id = "WifiManage",
    .name = "Wi-Fi",
    .icon = LV_SYMBOL_WIFI,
    .type = AppTypeSettings,
    .on_start = &app_start,
    .on_stop = &app_stop,
    .on_show = &app_show,
    .on_hide = &app_hide
};

} // namespace
