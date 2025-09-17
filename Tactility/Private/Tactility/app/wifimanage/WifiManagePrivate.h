#pragma once

#include "./View.h"
#include "./State.h"

#include <Tactility/app/App.h>

#include <Tactility/PubSub.h>
#include <Tactility/Mutex.h>
#include <Tactility/service/wifi/Wifi.h>

namespace tt::app::wifimanage {

class WifiManage final : public App {

    PubSub<service::wifi::WifiEvent>::SubscriptionHandle wifiSubscription = nullptr;
    Mutex mutex;
    Bindings bindings = { };
    State state;
    View view = View(&bindings, &state);
    bool isViewEnabled = false;

    void onWifiEvent(service::wifi::WifiEvent event);

public:

    WifiManage();

    void lock();
    void unlock();

    void onShow(AppContext& app, lv_obj_t* parent) override;
    void onHide(AppContext& app) override;

    Bindings& getBindings() { return bindings; }
    State& getState() { return state; }

    void requestViewUpdate();
};

} // namespace
