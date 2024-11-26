#pragma once

#include "Mutex.h"
#include "WifiManageView.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wifimanage {

typedef struct {
    PubSubSubscription* wifi_subscription;
    Mutex* mutex;
    WifiManageState state;
    WifiManageView view;
    bool view_enabled;
    WifiManageBindings bindings;
} WifiManage;

void lock(WifiManage* wifi);

void unlock(WifiManage* wifi);

void request_view_update(WifiManage* wifi);

} // namespace
