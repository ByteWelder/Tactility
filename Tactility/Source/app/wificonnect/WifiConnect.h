#pragma once

#include "Mutex.h"
#include "WifiConnectBindings.h"
#include "WifiConnectState.h"
#include "WifiConnectView.h"
#include "service/wifi/Wifi.h"

namespace tt::app::wificonnect {

typedef struct {
    PubSubSubscription* wifi_subscription;
    Mutex* mutex;
    WifiConnectState state;
    WifiConnectView view;
    bool view_enabled;
    WifiConnectBindings bindings;
} WifiConnect;

void lock(WifiConnect* wifi);

void unlock(WifiConnect* wifi);

void view_update(WifiConnect* wifi);

} // namespace