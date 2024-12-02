#pragma once

#include "Mutex.h"
#include "View.h"
#include "State.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wifimanage {

class WifiManage {

    PubSubSubscription* wifiSubscription = nullptr;
    Mutex mutex;
    tt::app::wifimanage::State state;
    tt::app::wifimanage::View view;
    Bindings bindings = { };
    bool isViewEnabled = false;

public:

    WifiManage();

    void lock();
    void unlock();

    void onShow(App& app, lv_obj_t* parent);
    void onHide(App& app);

    State& getState() { return state; }

    void requestViewUpdate();
};

} // namespace
