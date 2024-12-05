#pragma once

#include "Mutex.h"
#include "View.h"
#include "State.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wifimanage {

class WifiManage {

    PubSubSubscription* wifiSubscription = nullptr;
    Mutex mutex;
    Bindings bindings = { };
    State state;
    View view = View(&bindings, &state);
    bool isViewEnabled = false;

public:

    WifiManage();

    void lock();
    void unlock();

    void onShow(AppContext& app, lv_obj_t* parent);
    void onHide(AppContext& app);

    Bindings& getBindings() { return bindings; }
    State& getState() { return state; }

    void requestViewUpdate();
};

} // namespace
