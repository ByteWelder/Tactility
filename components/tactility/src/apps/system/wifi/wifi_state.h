#pragma once

#include <stdbool.h>

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
