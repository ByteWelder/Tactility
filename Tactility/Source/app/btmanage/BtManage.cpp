#include <lvgl/lvgl.h>

#include <Tactility/app/btmanage/BtManagePrivate.h>
#include <Tactility/app/btmanage/View.h>

#include <Tactility/Tactility.h>
#include <Tactility/app/AppContext.h>
#include <Tactility/app/AppManifest.h>

#include <lvgl/icons/shared.h>
#include <tactility/log.h>

namespace tt::app::btmanage {

constexpr auto* TAG = "BtManage";

extern const AppManifest manifest;

static void onBtToggled(bool requestOn) {
#if defined(CONFIG_BT_NIMBLE_ENABLED)
    Device* dev;
    if (device_get_first_by_type(&BLUETOOTH_TYPE, &dev) == ERROR_NONE) {
        bool radio_on = bluetooth::isRadioOnOrPending(dev);
        if (requestOn && !radio_on) {
            LOG_I(TAG, "Turning on");
            if (bluetooth::start(dev)) {
                // The driver only allocates its callback list once the device is started,
                // so the registration attempted in onShow() (while radio was off) was a
                // no-op. Register again now that the device is actually up.
                auto bt = std::static_pointer_cast<BtManage>(getCurrentApp());
                bt->registerDeviceCallback(dev);
            }
        } else if (!requestOn && radio_on) {
            LOG_I(TAG, "Turning off");
            bluetooth::stop(dev);
        }
        device_put(dev);
    } else {
        LOG_W(TAG, "Toggle: No bluetooth device found");
    }

#endif
}

static void onScanToggled(bool enabled) {
    Device* dev;
    if (device_get_first_active_by_type(&BLUETOOTH_TYPE, &dev) != ERROR_NONE) {
        LOG_W(TAG, "Scan: No bluetooth device found");
        return;
    }

    if (enabled) {
        bluetooth_scan_start(dev);
    } else {
        bluetooth_scan_stop(dev);
    }

    device_put(dev);
}

static void onConnectPeer(const std::array<uint8_t, 6>& addr, int profileId) {
    bluetooth::connect(addr, profileId);
}

static void onDisconnectPeer(const std::array<uint8_t, 6>& addr, int profileId) {
    bluetooth::disconnect(addr, profileId);
}

static void onPairPeer(const std::array<uint8_t, 6>& addr) {
    // Clicking an unrecognised scan result initiates a HID host connection.
    // Bond exchange happens automatically during the first connection.
    bluetooth::hidHostConnect(addr);
}

static void onForgetPeer(const std::array<uint8_t, 6>& addr) {
    bluetooth::unpair(addr);
}

BtManage::BtManage() {
    bindings = (Bindings) {
        .onBtToggled = onBtToggled,
        .onScanToggled = onScanToggled,
        .onConnectPeer = onConnectPeer,
        .onDisconnectPeer = onDisconnectPeer,
        .onPairPeer = onPairPeer,
        .onForgetPeer = onForgetPeer,
    };
}

void BtManage::lock() {
    mutex.lock();
}

void BtManage::unlock() {
    mutex.unlock();
}

void BtManage::requestViewUpdate() {
    lock();
    if (isViewEnabled) {
        lvgl_lock();
        view.update();
        lvgl_unlock();
    }
    unlock();
}

void BtManage::onBtEvent(const BtEvent& event) {
    auto radio_state = bluetooth::getRadioState();
    LOG_I(TAG, "Update with state %s", bluetooth::radioStateToString(radio_state));
    getState().setRadioState(radio_state);
    switch (event.type) {
        case BT_EVENT_SCAN_STARTED:
            getState().setScanning(true);
            break;
        case BT_EVENT_SCAN_FINISHED:
            getState().setScanning(false);
            getState().updateScanResults();
            getState().updatePairedPeers();
            break;
        case BT_EVENT_PEER_FOUND:
            getState().updateScanResults();
            break;
        case BT_EVENT_PAIR_RESULT:
            getState().updatePairedPeers();
            break;
        case BT_EVENT_PROFILE_STATE_CHANGED:
            getState().updateScanResults();
            getState().updatePairedPeers();
            break;
        case BT_EVENT_RADIO_STATE_CHANGED:
            if (event.radio_state == BT_RADIO_STATE_ON) {
                getState().updatePairedPeers();
                Device* dev = nullptr;
                if (device_get_first_active_by_type(&BLUETOOTH_TYPE, &dev) == ERROR_NONE && !bluetooth_is_scanning(dev)) {
                    bluetooth_scan_start(dev);
                }
                if (dev) {
                    device_put(dev);
                }
            }
            break;
        default:
            break;
    }

    requestViewUpdate();
}

static void onKernelBtEvent(Device* /*device*/, void* context, BtEvent event) {
    // BT event callbacks can fire from the NimBLE host task (e.g. DISCONNECT during
    // nimble_port_stop shutdown). Calling onBtEvent() synchronously from the NimBLE
    // task would block it on the LVGL mutex (held by the LVGL task waiting in
    // nimble_port_stop), creating a permanent deadlock. Dispatch to the main task so
    // the NimBLE host task is never blocked by BtManage's state updates or LVGL lock.
    auto* self = static_cast<BtManage*>(context);
    getMainDispatcher().dispatch([self, event] {
        self->onBtEvent(event);
    });
}

void BtManage::registerDeviceCallback(Device* dev) {
    lock();
    if (btDevice == dev) {
        bluetooth_add_event_callback(dev, this, onKernelBtEvent);
    }
    unlock();
}

void BtManage::onShow(AppContext& app, lv_obj_t* parent) {
    // Initialise state and view before subscribing to avoid incoming events
    // racing with state initialisation.
    state.setRadioState(bluetooth::getRadioState());
    Device* dev = nullptr;
    device_get_first_by_type(&BLUETOOTH_TYPE, &dev);

    state.setScanning(dev ? bluetooth_is_scanning(dev) : false);
    state.updateScanResults();
    state.updatePairedPeers();

    lock();
    isViewEnabled = true;
    view.init(app, parent);
    view.update();
    unlock();

    if (btDevice) {
        // Decrease refcount before re-ssignment
        device_put(btDevice);
    }

    btDevice = dev;
    if (btDevice) {
        bluetooth_add_event_callback(btDevice, this, onKernelBtEvent);
    }

    auto radio_state = bluetooth::getRadioState();
    bool can_scan = radio_state == bluetooth::RadioState::On;
    LOG_I(TAG, "Radio: %s, Scanning: %d, Can scan: %d",
        bluetooth::radioStateToString(radio_state),
        (int)(dev ? bluetooth_is_scanning(dev) : false),
        (int)can_scan);
    if (can_scan && dev && !bluetooth_is_scanning(dev)) {
        bluetooth_scan_start(dev);
    }
}

void BtManage::onHide(AppContext& app) {
    lock();
    if (btDevice) {
        bluetooth_remove_event_callback(btDevice, onKernelBtEvent);
        device_put(btDevice);
        btDevice = nullptr;
    }
    isViewEnabled = false;
    unlock();
}

extern const AppManifest manifest = {
    .appId = "BtManage",
    .appName = "Bluetooth",
    .appIcon = LVGL_ICON_SHARED_BLUETOOTH,
    .appCategory = Category::Settings,
    .createApp = create<BtManage>
};

LaunchId start() {
    return app::start(manifest.appId);
}

} // namespace tt::app::btmanage
