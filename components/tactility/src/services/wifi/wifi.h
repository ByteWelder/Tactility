#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_wifi.h"
#include "pubsub.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    /** Radio was turned on */
    WifiEventTypeRadioStateOn,
    /** Radio is turning on. */
    WifiEventTypeRadioStateOnPending,
    /** Radio is turned off */
    WifiEventTypeRadioStateOff,
    /** Radio is turning off */
    WifiEventTypeRadioStateOffPending,
    /** Started scanning for access points */
    WifiEventTypeScanStarted,
    /** Finished scanning for access points */ // TODO: 1 second validity
    WifiEventTypeScanFinished
} WifiEventType;

typedef struct {
    WifiEventType type;
} WifiEvent;

typedef struct {
    uint8_t ssid[33];
    int8_t rssi;
    wifi_auth_mode_t auth_mode;
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
 * @brief Returns the access points from the last scan (if any). It only contains public APs.
 * @param records the allocated buffer to store the records in
 * @param limit the maximum amount of records to store
 */
void wifi_get_scan_results(WifiApRecord records[], uint16_t limit, uint16_t* result_count);

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
