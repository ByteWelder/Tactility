#pragma once

#include "Mutex.h"
#include "WifiManageView.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wifimanage {

struct WifiManage {
    PubSubSubscription* wifi_subscription = nullptr;
    Mutex* mutex = nullptr;
    WifiManageState state;
    WifiManageView view;
    WifiManageBindings bindings;
    bool view_enabled;
};

void lock(WifiManage* wifi);

void unlock(WifiManage* wifi);

void request_view_update(WifiManage* wifi);

} // namespace
