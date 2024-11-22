#pragma once

#include "Mutex.h"
#include "WifiConnectBindings.h"
#include "WifiConnectState.h"
#include "WifiConnectView.h"
#include "Services/Wifi/Wifi.h"

namespace tt::app::wifi_connect {

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
