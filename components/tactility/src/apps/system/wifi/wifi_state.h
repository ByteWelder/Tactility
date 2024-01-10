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

typedef enum {
    WIFI_SCREEN_MAIN,
    WIFI_SCREEN_CONNECT
} WifiActiveScreen;

/**
 * View's state
 */
typedef struct {
    bool scanning;
    WifiRadioState radio_state;
    WifiActiveScreen active_screen;
    uint8_t connect_ssid[33];
} WifiState;

#ifdef __cplusplus
}
#endif
