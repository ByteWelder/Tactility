#pragma once

#include "Bindings.h"
#include "State.h"
#include "View.h"

#include "Mutex.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wificonnect {

class WifiConnect {
    PubSubSubscription* wifiSubscription;
    Mutex mutex;
    State state;
    View view;
    bool view_enabled = false;
    Bindings bindings = {
        .onConnectSsid = nullptr,
        .onConnectSsidContext = nullptr
    };

public:

    WifiConnect();
    ~WifiConnect();

    void lock();
    void unlock();

    void onShow(App& app, lv_obj_t* parent);
    void onHide(App& app);

    State& getState() { return state; }
    Bindings& getBindings() { return bindings; }
    View& getView() { return view; }


    void requestViewUpdate();
};

void lock(WifiConnect* wifi);

void unlock(WifiConnect* wifi);

void view_update(WifiConnect* wifi);

} // namespace
