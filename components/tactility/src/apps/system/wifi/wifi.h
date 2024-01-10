#pragma once

#include "mutex.h"
#include "services/wifi/wifi.h"
#include "wifi_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FuriPubSubSubscription* wifi_subscription;
    FuriMutex* mutex;
    WifiState state;
    WifiView view;
    WifiBindings bindings;
} Wifi;

void wifi_lock(Wifi* wifi);

void wifi_unlock(Wifi* wifi);

#ifdef __cplusplus
}
#endif
