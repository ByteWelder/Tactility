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
