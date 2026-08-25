#pragma once

#include "./View.h"
#include "./State.h"

#include <Tactility/Mutex.h>
#include <Tactility/bluetooth/Bluetooth.h>
#include <tactility/drivers/bluetooth.h>

namespace tt::app::btmanage {

struct Context {
    uint32_t appInstanceId;
    Mutex mutex;
    Bindings bindings {};
    State state;
    View view = View(&bindings, &state);
    Device* btDevice = nullptr;
    BtEventSubscription btEventSub {};

    void lock() { mutex.lock(); }
    void unlock() { mutex.unlock(); }
};

void onBtEvent(Context* ctx, const struct BtEvent& event);
void requestViewUpdate(Context* ctx);

} // namespace tt::app::btmanage
