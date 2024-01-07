#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pubsub.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    WifiEventTypeRadioStateOn,
    WifiEventTypeRadioStateOnPending,
    WifiEventTypeRadioStateOff,
    WifiEventTypeRadioStateOffPending,
    WifiEventTypeScanStarted,
    WifiEventTypeScanFinished
} WifiEventType;

typedef struct {
    WifiEventType type;
} WifiEvent;

typedef struct {
    uint8_t ssid[33];
    int8_t rssi;
} WifiApRecord;

/**
 * @brief Get wifi pubsub
 * @return FuriPubSub*
 */
FuriPubSub* wifi_get_pubsub();

/**
 * @brief Request scanning update.
 */
void wifi_scan();

/**
 * @param records the allocated buffer to store the records in
 * @param limit the maximum amount of records to store
 */
void wifi_get_scan_results(WifiApRecord records[], uint8_t limit, uint8_t* count);

/**
 * @brief Overrides the default scan result size of 16.
 * @param records the record limit for the scan result (84 bytes per record!)
 */
void wifi_set_scan_records(uint16_t records);

/**
 * @brief Enable/disable the radio. Ignores input if desired state matches current state.
 * @param enabled
 */
void wifi_set_enabled(bool enabled);

/**
 * @return true if the wifi radio is fully operational in STA mode.
 */
bool wifi_get_enabled();

#ifdef __cplusplus
}
#endif
