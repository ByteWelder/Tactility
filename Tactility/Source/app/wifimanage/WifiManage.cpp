#include "app/wifimanage/WifiManagePrivate.h"
#include "app/wifimanage/View.h"
#include "app/wifimanage/State.h"

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
    auto appContext = service::loader::getCurrentAppContext();
    if (appContext != nullptr && appContext->getManifest().id == manifest.id) {
        return std::static_pointer_cast<WifiManage>(appContext->getApp());
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
    mutex.lock();
}

void WifiManage::unlock() {
    mutex.unlock();
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
    auto* event = (service::wifi::Event*)message;
    auto* wifi = (WifiManage*)context;
    auto radio_state = service::wifi::getRadioState();
    TT_LOG_I(TAG, "Update with state %s", service::wifi::radioStateToString(radio_state));
    wifi->getState().setRadioState(radio_state);
    switch (event->type) {
        case tt::service::wifi::EventType::ScanStarted:
            wifi->getState().setScanning(true);
            break;
        case tt::service::wifi::EventType::ScanFinished:
            wifi->getState().setScanning(false);
            wifi->getState().updateApRecords();
            break;
        case tt::service::wifi::EventType::RadioStateOn:
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
    wifiSubscription = service::wifi::getPubsub()->subscribe(&wifiManageEventCallback, this);

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

    service::wifi::RadioState radio_state = service::wifi::getRadioState();
    bool can_scan = radio_state == service::wifi::RadioState::On ||
        radio_state == service::wifi::RadioState::ConnectionPending ||
        radio_state == service::wifi::RadioState::ConnectionActive;
    TT_LOG_I(TAG, "%s %d", service::wifi::radioStateToString(radio_state), (int)service::wifi::isScanning());
    if (can_scan && !service::wifi::isScanning()) {
        service::wifi::scan();
    }
}

void WifiManage::onHide(TT_UNUSED AppContext& app) {
    lock();
    service::wifi::getPubsub()->unsubscribe(wifiSubscription);
    wifiSubscription = nullptr;
    isViewEnabled = false;
    unlock();
}

extern const AppManifest manifest = {
    .id = "WifiManage",
    .name = "Wi-Fi",
    .icon = LV_SYMBOL_WIFI,
    .type = Type::Settings,
    .createApp = create<WifiManage>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
