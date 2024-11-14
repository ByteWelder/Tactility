#pragma once

#include "app.h"
#include "services/wifi/wifi.h"
#include "services/wifi/wifi_settings.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * View's state
 */
typedef struct {
    WifiApSettings settings;
    bool connection_error;
    bool is_connecting;
} WifiConnectState;

#ifdef __cplusplus
}
#endif
