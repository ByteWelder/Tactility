#pragma once

#include <stdbool.h>
#include "services/wifi/wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SCAN_AP_RECORD_COUNT 16

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
    uint8_t connect_ssid[33];
    WifiApRecord ap_records[WIFI_SCAN_AP_RECORD_COUNT];
    uint16_t ap_records_count;
} WifiManageState;

#ifdef __cplusplus
}
#endif
