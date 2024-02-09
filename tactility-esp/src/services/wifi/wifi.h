#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pubsub.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef ESP_PLATFORM
#include "esp_wifi.h"
#endif

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
    WifiEventTypeScanFinished,
    WifiEventTypeDisconnected,
    WifiEventTypeConnectionPending,
    WifiEventTypeConnectionSuccess,
    WifiEventTypeConnectionFailed
} WifiEventType;

typedef enum {
    WIFI_RADIO_ON_PENDING,
    WIFI_RADIO_ON,
    WIFI_RADIO_CONNECTION_PENDING,
    WIFI_RADIO_CONNECTION_ACTIVE,
    WIFI_RADIO_OFF_PENDING,
    WIFI_RADIO_OFF
} WifiRadioState;

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
 * @return PubSub*
 */
PubSub* wifi_get_pubsub();

WifiRadioState wifi_get_radio_state();
/**
 * @brief Request scanning update. Returns immediately. Results are through pubsub.
 */
void wifi_scan();

bool wifi_is_scanning();

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
 * @brief Connect to a network. Disconnects any existing connection.
 * Returns immediately but runs in the background. Results are through pubsub.
 * @param ssid
 * @param password
 */
void wifi_connect(const char* ssid, const char _Nullable password[64]);

/**
 * @brief Disconnect from the access point. Doesn't have any effect when not connected.
 */
void wifi_disconnect();

const char* wifi_get_status_icon_for_rssi(int rssi, bool secure);

#ifdef __cplusplus
}
#endif
