#pragma once
#include "services/wifi/wifi.h"
#include "mutex.h"
#include "wifi_state.h"
#include "wifi_view.h"

typedef struct {
    FuriPubSubSubscription* wifi_subscription;
    FuriMutex* mutex;
    WifiState state;
    WifiView view;
} Wifi;

void wifi_lock(Wifi* wifi);

void wifi_unlock(Wifi* wifi);
