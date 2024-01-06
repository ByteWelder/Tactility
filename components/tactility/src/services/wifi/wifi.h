#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pubsub.h"

typedef enum {
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

#ifdef __cplusplus
}
#endif
