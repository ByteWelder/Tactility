#pragma once

#include "Mutex.h"
#include "services/wifi/wifi.h"
#include "wifi_connect_bindings.h"
#include "wifi_connect_state.h"
#include "wifi_connect_view.h"

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
