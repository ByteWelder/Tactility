#pragma once

#include "mutex.h"
#include "services/wifi/wifi.h"
#include "wifi_manage_view.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FuriPubSubSubscription* wifi_subscription;
    FuriMutex* mutex;
    WifiManageState state;
    WifiManageView view;
    bool view_enabled;
    WifiManageBindings bindings;
} WifiManage;

void wifi_manage_lock(WifiManage* wifi);

void wifi_manage_unlock(WifiManage* wifi);

void wifi_manage_request_view_update(WifiManage* wifi);

#ifdef __cplusplus
}
#endif
