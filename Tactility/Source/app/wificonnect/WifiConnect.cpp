#include "app/wificonnect/WifiConnectPrivate.h"

#include "app/AppContext.h"
#include "TactilityCore.h"
#include "service/loader/Loader.h"
#include "service/wifi/Wifi.h"
#include "lvgl/LvglSync.h"

namespace tt::app::wificonnect {

#define TAG "wifi_connect"
#define WIFI_CONNECT_PARAM_SSID "ssid" // String
#define WIFI_CONNECT_PARAM_PASSWORD "password" // String

extern const AppManifest manifest;

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
std::shared_ptr<WifiConnect> _Nullable optWifiConnect() {
    auto appContext = service::loader::getCurrentAppContext();
    if (appContext != nullptr && appContext->getManifest().id == manifest.id) {
        return std::static_pointer_cast<WifiConnect>(appContext->getApp());
    } else {
        return nullptr;
    }
}

static void eventCallback(const void* message, void* context) {
    auto* event = static_cast<const service::wifi::Event*>(message);
    auto* wifi = static_cast<WifiConnect*>(context);
    State& state = wifi->getState();
    switch (event->type) {
        case service::wifi::EventType::ConnectionFailed:
            if (state.isConnecting()) {
                state.setConnecting(false);
                state.setConnectionError(true);
                wifi->requestViewUpdate();
            }
            break;
        case service::wifi::EventType::ConnectionSuccess:
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
    wifiSubscription = service::wifi::getPubsub()->subscribe(&eventCallback, this);
    bindings = (Bindings) {
        .onConnectSsid = onConnect,
        .onConnectSsidContext = this,
    };
}

WifiConnect::~WifiConnect() {
    service::wifi::getPubsub()->unsubscribe(wifiSubscription);
}

void WifiConnect::lock() {
    mutex.lock();
}

void WifiConnect::unlock() {
    mutex.unlock();
}

void WifiConnect::requestViewUpdate() {
    lock();
    if (view_enabled) {
        if (lvgl::lock(1000)) {
            view.update();
            lvgl::unlock();
        } else {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "LVGL");
        }
    }
    unlock();
}

void WifiConnect::onShow(AppContext& app, lv_obj_t* parent) {
    lock();
    view_enabled = true;
    view.init(app, parent);
    view.update();
    unlock();
}

void WifiConnect::onHide(TT_UNUSED AppContext& app) {
    // No need to lock view, as this is called from within Gui's LVGL context
    lock();
    view_enabled = false;
    unlock();
}

extern const AppManifest manifest = {
    .id = "WifiConnect",
    .name = "Wi-Fi Connect",
    .icon = LV_SYMBOL_WIFI,
    .type = Type::Hidden,
    .createApp = create<WifiConnect>
};

void start(const std::string& ssid, const std::string& password) {
    auto parameters = std::make_shared<Bundle>();
    parameters->putString(WIFI_CONNECT_PARAM_SSID, ssid);
    parameters->putString(WIFI_CONNECT_PARAM_PASSWORD, password);
    service::loader::startApp(manifest.id, parameters);
}

bool optSsidParameter(const std::shared_ptr<const Bundle>& bundle, std::string& ssid) {
    return bundle->optString(WIFI_CONNECT_PARAM_SSID, ssid);
}

bool optPasswordParameter(const std::shared_ptr<const Bundle>& bundle, std::string& password) {
    return bundle->optString(WIFI_CONNECT_PARAM_PASSWORD, password);
}

} // namespace
