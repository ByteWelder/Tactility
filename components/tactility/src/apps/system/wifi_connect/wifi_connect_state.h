#pragma once

#include <stdbool.h>
#include "services/wifi/wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * View's state
 */
typedef struct {
    WifiRadioState radio_state;
    uint8_t connect_ssid[33];
    bool connection_error;
} WifiConnectState;

#ifdef __cplusplus
}
#endif
