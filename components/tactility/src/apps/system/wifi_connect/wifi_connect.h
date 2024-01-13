#pragma once

#include "mutex.h"
#include "services/wifi/wifi.h"
#include "wifi_connect_bindings.h"
#include "wifi_connect_state.h"
#include "wifi_connect_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FuriPubSubSubscription* wifi_subscription;
    FuriMutex* mutex;
    WifiConnectState state;
    WifiConnectView view;
    bool view_enabled;
    WifiConnectBindings bindings;
} WifiConnect;

void wifi_connect_lock(WifiConnect* wifi);

void wifi_connect_unlock(WifiConnect* wifi);

void wifi_connect_request_view_update(WifiConnect* wifi);

#ifdef __cplusplus
}
#endif
