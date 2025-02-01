#pragma once

#include "Tactility/app/App.h"
#include "Tactility/app/wificonnect/Bindings.h"
#include "Tactility/app/wificonnect/State.h"
#include "Tactility/app/wificonnect/View.h"

#include <Tactility/Mutex.h>
#include <Tactility/service/wifi/Wifi.h>

namespace tt::app::wificonnect {

class WifiConnect : public App {

private:

    Mutex mutex;
    State state;
    Bindings bindings = {
        .onConnectSsid = nullptr,
        .onConnectSsidContext = nullptr
    };
    View view = View(&bindings, &state);
    PubSub::SubscriptionHandle wifiSubscription;
    bool view_enabled = false;

public:

    WifiConnect();
    ~WifiConnect();

    void lock();
    void unlock();

    void onShow(AppContext& app, lv_obj_t* parent) override;
    void onHide(AppContext& app) override;

    State& getState() { return state; }
    Bindings& getBindings() { return bindings; }
    View& getView() { return view; }


    void requestViewUpdate();
};

/**
 * Start the app with optional pre-filled fields.
 */
void start(const std::string& ssid = "", const std::string& password = "");

bool optSsidParameter(const std::shared_ptr<const Bundle>& bundle, std::string& ssid);

bool optPasswordParameter(const std::shared_ptr<const Bundle>& bundle, std::string& password);

} // namespace
