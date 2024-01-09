#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_RADIO_ON,
    WIFI_RADIO_ON_PENDING,
    WIFI_RADIO_OFF,
    WIFI_RADIO_OFF_PENDING,
} WifiRadioState;

/**
 * View's state
 */
typedef struct {
    bool scanning;
    WifiRadioState radio_state;
} WifiState;

#ifdef __cplusplus
}
#endif
