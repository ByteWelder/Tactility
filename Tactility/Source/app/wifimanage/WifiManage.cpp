#include "WifiManage.h"
#include "View.h"
#include "State.h"

#include "app/AppContext.h"
#include "app/wifiapsettings/WifiApSettings.h"
#include "TactilityCore.h"
#include "service/loader/Loader.h"
#include "service/wifi/WifiSettings.h"
#include "lvgl/LvglSync.h"
#include "app/wificonnect/WifiConnect.h"

namespace tt::app::wifimanage {

#define TAG "wifi_manage"

extern const AppManifest manifest;

/** Returns the app data if the app is active. Note that this could clash if the same app is started twice and a background thread is slow. */
std::shared_ptr<WifiManage> _Nullable optWifiManage() {
    app::AppContext* app = service::loader::getCurrentApp();
    if (app->getManifest().id == manifest.id) {
        return std::static_pointer_cast<WifiManage>(app->getData());
    } else {
        return nullptr;
    }
}

static void onConnect(const char* ssid) {
    service::wifi::settings::WifiApSettings settings;
    if (service::wifi::settings::load(ssid, &settings)) {
        TT_LOG_I(TAG, "Connecting with known credentials");
        service::wifi::connect(&settings, false);
    } else {
        TT_LOG_I(TAG, "Starting connection dialog");
        wificonnect::start(ssid);
    }
}

static void onShowApSettings(const char* ssid) {
    wifiapsettings::start(ssid);
}

static void onDisconnect() {
    service::wifi::disconnect();
}

static void onWifiToggled(bool enabled) {
    service::wifi::setEnabled(enabled);
}

static void onConnectToHidden() {
    wificonnect::start();
}

WifiManage::WifiManage() {
    bindings = (Bindings) {
        .onWifiToggled = onWifiToggled,
        .onConnectSsid = onConnect,
        .onDisconnect = onDisconnect,
        .onShowApSettings = onShowApSettings,
        .onConnectToHidden = onConnectToHidden
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
            view.update();
            lvgl::unlock();
        } else {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED_FMT, "LVGL");
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

void WifiManage::onShow(AppContext& app, lv_obj_t* parent) {
    auto wifi_pubsub = service::wifi::getPubsub();
    wifiSubscription = tt_pubsub_subscribe(wifi_pubsub, &wifiManageEventCallback, this);

    // State update (it has its own locking)
    state.setRadioState(service::wifi::getRadioState());
    state.setScanning(service::wifi::isScanning());
    state.updateApRecords();

    // View update
    lock();
    isViewEnabled = true;
    state.setConnectSsid("Connected"); // TODO update with proper SSID
    view.init(app, parent);
    view.update();
    unlock();

    service::wifi::WifiRadioState radio_state = service::wifi::getRadioState();
    bool can_scan = radio_state == service::wifi::WIFI_RADIO_ON ||
        radio_state == service::wifi::WIFI_RADIO_CONNECTION_PENDING ||
        radio_state == service::wifi::WIFI_RADIO_CONNECTION_ACTIVE;
    TT_LOG_I(TAG, "%d %d", radio_state, service::wifi::isScanning());
    if (can_scan && !service::wifi::isScanning()) {
        service::wifi::scan();
    }
}

void WifiManage::onHide(TT_UNUSED AppContext& app) {
    lock();
    auto wifi_pubsub = service::wifi::getPubsub();
    tt_pubsub_unsubscribe(wifi_pubsub, wifiSubscription);
    wifiSubscription = nullptr;
    isViewEnabled = false;
    unlock();
}

// region Manifest methods

static void onStart(AppContext& app) {
    auto wifi = std::make_shared<WifiManage>();
    app.setData(wifi);
}

static void onShow(AppContext& app, lv_obj_t* parent) {
    auto wifi = std::static_pointer_cast<WifiManage>(app.getData());
    wifi->onShow(app, parent);
}

static void onHide(AppContext& app) {
    auto wifi = std::static_pointer_cast<WifiManage>(app.getData());
    wifi->onHide(app);
}

// endregion

extern const AppManifest manifest = {
    .id = "WifiManage",
    .name = "Wi-Fi",
    .icon = LV_SYMBOL_WIFI,
    .type = TypeSettings,
    .onStart = onStart,
    .onShow = onShow,
    .onHide = onHide
};

} // namespace
