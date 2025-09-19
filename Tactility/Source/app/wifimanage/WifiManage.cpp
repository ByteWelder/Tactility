#include <Tactility/app/wifimanage/WifiManagePrivate.h>
#include <Tactility/app/wifimanage/View.h>

#include <Tactility/app/AppContext.h>
#include <Tactility/app/wifiapsettings/WifiApSettings.h>
#include <Tactility/app/wificonnect/WifiConnect.h>
#include <Tactility/lvgl/LvglSync.h>
#include <Tactility/service/loader/Loader.h>

namespace tt::app::wifimanage {

constexpr auto TAG = "WifiManage";

extern const AppManifest manifest;

static void onConnect(const std::string& ssid) {
    service::wifi::settings::WifiApSettings settings;
    if (service::wifi::settings::load(ssid, settings)) {
        TT_LOG_I(TAG, "Connecting with known credentials");
        service::wifi::connect(settings, false);
    } else {
        TT_LOG_I(TAG, "Starting connection dialog");
        wificonnect::start(ssid);
    }
}

static void onShowApSettings(const std::string& ssid) {
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

void WifiManage::onWifiEvent(service::wifi::WifiEvent event) {
    auto radio_state = service::wifi::getRadioState();
    TT_LOG_I(TAG, "Update with state %s", service::wifi::radioStateToString(radio_state));
    getState().setRadioState(radio_state);
    switch (event) {
        using enum service::wifi::WifiEvent;
        case ScanStarted:
            getState().setScanning(true);
            break;
        case ScanFinished:
            getState().setScanning(false);
            getState().updateApRecords();
            break;
        case RadioStateOn:
            if (!service::wifi::isScanning()) {
                service::wifi::scan();
            }
            break;
        default:
            break;
    }

    requestViewUpdate();
}

void WifiManage::onShow(AppContext& app, lv_obj_t* parent) {
    wifiSubscription = service::wifi::getPubsub()->subscribe([this](auto event) {
        onWifiEvent(event);
    });

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
    .category = Category::Settings,
    .createApp = create<WifiManage>
};

void start() {
    service::loader::startApp(manifest.id);
}

} // namespace
