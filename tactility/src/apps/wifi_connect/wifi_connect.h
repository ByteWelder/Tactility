#pragma once

#include "mutex.h"
#include "services/wifi/wifi.h"
#include "wifi_connect_bindings.h"
#include "wifi_connect_state.h"
#include "wifi_connect_view.h"

typedef struct {
    PubSubSubscription* wifi_subscription;
    Mutex* mutex;
    WifiConnectState state;
    WifiConnectView view;
    bool view_enabled;
    WifiConnectBindings bindings;
} WifiConnect;

void wifi_connect_lock(WifiConnect* wifi);

void wifi_connect_unlock(WifiConnect* wifi);

void wifi_connect_request_view_update(WifiConnect* wifi);
