#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pubsub.h"
#include <stdbool.h>

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

void wifi_set_enabled(bool enabled);

bool wifi_get_enabled();

#ifdef __cplusplus
}
#endif
