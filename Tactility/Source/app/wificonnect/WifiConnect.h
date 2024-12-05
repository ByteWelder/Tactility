#pragma once

#include "Bindings.h"
#include "State.h"
#include "View.h"

#include "Mutex.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wificonnect {

class WifiConnect {
    Mutex mutex;
    State state;
    Bindings bindings = {
        .onConnectSsid = nullptr,
        .onConnectSsidContext = nullptr
    };
    View view = View(&bindings, &state);
    PubSubSubscription* wifiSubscription;
    bool view_enabled = false;

public:

    WifiConnect();
    ~WifiConnect();

    void lock();
    void unlock();

    void onShow(AppContext& app, lv_obj_t* parent);
    void onHide(AppContext& app);

    State& getState() { return state; }
    Bindings& getBindings() { return bindings; }
    View& getView() { return view; }


    void requestViewUpdate();
};

void lock(WifiConnect* wifi);

void unlock(WifiConnect* wifi);

void view_update(WifiConnect* wifi);

} // namespace
