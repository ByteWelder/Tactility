#include "WifiManage.h"

#include "app/App.h"
#include "app/wificonnect/Parameters.h"
#include "TactilityCore.h"
#include "service/loader/Loader.h"
#include "service/wifi/WifiSettings.h"
#include "lvgl/LvglSync.h"
#include "View.h"
#include "State.h"

namespace tt::app::wifimanage {

#define TAG "wifi_manage"

static void onConnect(const char* ssid) {
    service::wifi::settings::WifiApSettings settings;
    if (service::wifi::settings::load(ssid, &settings)) {
        TT_LOG_I(TAG, "Connecting with known credentials");
        service::wifi::connect(&settings, false);
    } else {
        TT_LOG_I(TAG, "Starting connection dialog");
        Bundle bundle;
        bundle.putString(WIFI_CONNECT_PARAM_SSID, ssid);
        bundle.putString(WIFI_CONNECT_PARAM_PASSWORD, "");
        service::loader::startApp("WifiConnect", false, bundle);
    }
}

static void onDisconnect() {
    service::wifi::disconnect();
}

static void onWifiToggled(bool enabled) {
    service::wifi::setEnabled(enabled);
}

WifiManage::WifiManage() {
    bindings = (Bindings) {
        .onWifiToggled = &onWifiToggled,
        .onConnectSsid = &onConnect,
        .onDisconnect = &onDisconnect
    };
}

void WifiManage::lock() {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
}

void WifiManage::unlock() {
    tt_check(mutex.release() == TtStatusOk);
}

void WifiManage::requestViewUpdate() {
    lock();
    if (isViewEnabled) {
        if (lvgl::lock(1000)) {
            view.update(&bindings, &state);
            lvgl::unlock();
        } else {
            TT_LOG_E(TAG, "failed to lock lvgl");
        }
    }
    unlock();
}

static void wifiManageEventCallback(const void* message, void* context) {
    auto* event = (service::wifi::WifiEvent*)message;
    auto* wifi = (WifiManage*)context;
    TT_LOG_I(TAG, "Update with state %d", service::wifi::getRadioState());
    wifi->getState().setRadioState(service::wifi::getRadioState());
    switch (event->type) {
        case tt::service::wifi::WifiEventTypeScanStarted:
            wifi->getState().setScanning(true);
            break;
        case tt::service::wifi::WifiEventTypeScanFinished:
            wifi->getState().setScanning(false);
            wifi->getState().updateApRecords();
            break;
        case tt::service::wifi::WifiEventTypeRadioStateOn:
            if (!service::wifi::isScanning()) {
                service::wifi::scan();
            }
            break;
        default:
            break;
    }

    wifi->requestViewUpdate();
}

void WifiManage::onShow(App& app, lv_obj_t* parent) {
    PubSub* wifi_pubsub = service::wifi::getPubsub();
    wifiSubscription = tt_pubsub_subscribe(wifi_pubsub, &wifiManageEventCallback, this);

    // State update (it has its own locking)
    state.setRadioState(service::wifi::getRadioState());
    state.setScanning(service::wifi::isScanning());
    state.updateApRecords();

    // View update
    lock();
    isViewEnabled = true;
    state.setConnectSsid("Connected"); // TODO update with proper SSID
    view.init(app, &bindings, parent);
    view.update(&bindings, &state);
    unlock();

    service::wifi::WifiRadioState radio_state = service::wifi::getRadioState();
    bool can_scan = radio_state == service::wifi::WIFI_RADIO_ON ||
        radio_state == service::wifi::WIFI_RADIO_CONNECTION_PENDING ||
        radio_state == service::wifi::WIFI_RADIO_CONNECTION_ACTIVE;
    if (can_scan && !service::wifi::isScanning()) {
        service::wifi::scan();
    }
}

void WifiManage::onHide(TT_UNUSED App& app) {
    lock();
    PubSub* wifi_pubsub = service::wifi::getPubsub();
    tt_pubsub_unsubscribe(wifi_pubsub, wifiSubscription);
    wifiSubscription = nullptr;
    isViewEnabled = false;
    unlock();
}

// region Manifest methods

static void onStart(App& app) {
    auto* wifi = new WifiManage();
    app.setData(wifi);
}

static void onStop(App& app) {
    auto* wifi = (WifiManage*)app.getData();
    tt_assert(wifi != nullptr);
    delete wifi;
}

static void onShow(App& app, lv_obj_t* parent) {
    auto* wifi = (WifiManage*)app.getData();
    wifi->onShow(app, parent);
}

static void onHide(App& app) {
    auto* wifi = (WifiManage*)app.getData();
    wifi->onHide(app);
}


// endregion

extern const Manifest manifest = {
    .id = "WifiManage",
    .name = "Wi-Fi",
    .icon = LV_SYMBOL_WIFI,
    .type = TypeSettings,
    .onStart = onStart,
    .onStop = onStop,
    .onShow = onShow,
    .onHide = onHide
};

} // namespace
