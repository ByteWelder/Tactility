#include "WifiConnect.h"

#include "app/App.h"
#include "TactilityCore.h"
#include "service/loader/Loader.h"
#include "service/wifi/Wifi.h"
#include "lvgl/LvglSync.h"

namespace tt::app::wificonnect {

#define TAG "wifi_connect"

static void eventCallback(const void* message, void* context) {
    auto* event = static_cast<const service::wifi::WifiEvent*>(message);
    auto* wifi = static_cast<WifiConnect*>(context);
    State& state = wifi->getState();
    switch (event->type) {
        case service::wifi::WifiEventTypeConnectionFailed:
            if (state.isConnecting()) {
                state.setConnecting(false);
                state.setConnectionError(true);
                wifi->requestViewUpdate();
            }
            break;
        case service::wifi::WifiEventTypeConnectionSuccess:
            if (wifi->getState().isConnecting()) {
                state.setConnecting(false);
                service::loader::stopApp();
            }
            break;
        default:
            break;
    }
    wifi->requestViewUpdate();
}

static void onConnect(const service::wifi::settings::WifiApSettings* ap_settings, bool remember, TT_UNUSED void* parameter) {
    auto* wifi = static_cast<WifiConnect*>(parameter);
    wifi->getState().setApSettings(ap_settings);
    wifi->getState().setConnecting(true);
    service::wifi::connect(ap_settings, remember);
}

WifiConnect::WifiConnect() {
    PubSub* wifi_pubsub = service::wifi::getPubsub();
    wifiSubscription = tt_pubsub_subscribe(wifi_pubsub, &eventCallback, this);
    bindings = (Bindings) {
        .onConnectSsid = onConnect,
        .onConnectSsidContext = this,
    };
}

WifiConnect::~WifiConnect() {
    PubSub* pubsub = service::wifi::getPubsub();
    tt_pubsub_unsubscribe(pubsub, wifiSubscription);
}

void WifiConnect::lock() {
    tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
}

void WifiConnect::unlock() {
    tt_check(mutex.release() == TtStatusOk);
}

void WifiConnect::requestViewUpdate() {
    lock();
    if (view_enabled) {
        if (lvgl::lock(1000)) {
            view.update(&bindings, &state);
            lvgl::unlock();
        } else {
            TT_LOG_E(TAG, "Failed to lock lvgl");
        }
    }
    unlock();
}

void WifiConnect::onShow(App& app, lv_obj_t* parent) {
    lock();
    view_enabled = true;
    view.init(app, this, parent);
    view.update(&bindings, &state);
    unlock();
}

void WifiConnect::onHide(App& app) {
    // No need to lock view, as this is called from within Gui's LVGL context
    lock();
    view_enabled = false;
    unlock();
}

static void onShow(App& app, lv_obj_t* parent) {
    auto* wifi = static_cast<WifiConnect*>(app.getData());
    wifi->onShow(app, parent);
}

static void onHide(App& app) {
    auto* wifi = static_cast<WifiConnect*>(app.getData());
    wifi->onHide(app);
}

static void onStart(App& app) {
    auto* wifi_connect = new WifiConnect();
    app.setData(wifi_connect);
}

static void onStop(App& app) {
    auto* wifi_connect = static_cast<WifiConnect*>(app.getData());
    tt_assert(wifi_connect != nullptr);
    delete wifi_connect;
}

extern const Manifest manifest = {
    .id = "WifiConnect",
    .name = "Wi-Fi Connect",
    .icon = LV_SYMBOL_WIFI,
    .type = TypeSettings,
    .onStart = &onStart,
    .onStop = &onStop,
    .onShow = &onShow,
    .onHide = &onHide
};

} // namespace
