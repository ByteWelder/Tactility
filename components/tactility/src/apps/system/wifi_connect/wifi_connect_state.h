#pragma once

#include <stdbool.h>
#include "app.h"
#include "services/wifi/wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * View's state
 */
typedef struct {
    WifiRadioState radio_state;
    bool connection_error;
} WifiConnectState;

#ifdef __cplusplus
}
#endif
