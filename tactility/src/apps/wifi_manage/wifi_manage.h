#pragma once

#include "Mutex.h"
#include "services/wifi/wifi.h"
#include "wifi_manage_view.h"

typedef struct {
    PubSubSubscription* wifi_subscription;
    Mutex* mutex;
    WifiManageState state;
    WifiManageView view;
    bool view_enabled;
    WifiManageBindings bindings;
} WifiManage;

void wifi_manage_lock(WifiManage* wifi);

void wifi_manage_unlock(WifiManage* wifi);

void wifi_manage_request_view_update(WifiManage* wifi);
