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
    WifiManageBindings bindings;
} WifiManage;

void wifi_manage_lock(WifiManage* wifi);

void wifi_manage_unlock(WifiManage* wifi);

#ifdef __cplusplus
}
#endif
