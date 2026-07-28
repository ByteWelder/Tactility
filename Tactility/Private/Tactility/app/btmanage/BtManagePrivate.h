#pragma once

#include "./View.h"
#include "./State.h"

#include <Tactility/app/App.h>
#include <Tactility/Mutex.h>
#include <Tactility/bluetooth/Bluetooth.h>
#include <tactility/drivers/bluetooth.h>

#include <atomic>
#include <memory>

namespace tt::app::btmanage {

class BtManage final : public App {

    Mutex mutex;
    Bindings bindings = { };
    State state;
    View view = View(&bindings, &state);
    bool isViewEnabled = false;
    Device* btDevice = nullptr;
    bool callbackRegistered = false;

    // Bumped by onHide() to invalidate any BT event already dispatched to the main
    // task for this show/hide session (BtManage is reused across hide/show cycles -
    // e.g. launching BtPeerSettings pushes it on top and hides this instance without
    // destroying it). Kept in its own heap allocation, independent of BtManage's
    // lifetime, so a dispatched callback can check it without touching a possibly
    // already-destroyed `this`.
    std::shared_ptr<std::atomic<int>> generation = std::make_shared<std::atomic<int>>(0);

public:

    void onBtEvent(const struct BtEvent& event);

    BtManage();

    void lock();
    void unlock();

    void onShow(AppContext& app, lv_obj_t* parent) override;
    void onHide(AppContext& app) override;

    Bindings& getBindings() { return bindings; }
    State& getState() { return state; }

    void requestViewUpdate();

    std::shared_ptr<std::atomic<int>> getGeneration() const { return generation; }

    // Re-attempts registering the device event callback. Needed because the BLE driver
    // only allocates its callback list while the device is started/on: a registration
    // attempted while the radio is off silently no-ops, so this must be called again
    // right after a successful bluetooth::start(). Idempotent: no-ops if already
    // registered for this device, so it's safe to call from both onShow() and here.
    void registerDeviceCallback(Device* dev);

    // Call after bluetooth::stop(): the driver frees its callback list on stop, so the
    // registration state must be cleared here too, without touching the (now-dangling)
    // driver-side list.
    void forgetCallbackRegistration();
};

} // namespace tt::app::btmanage
