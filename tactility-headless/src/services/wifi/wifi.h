#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "pubsub.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef ESP_PLATFORM
#include "esp_wifi.h"
#else
#include <stdint.h>
// From esp_wifi_types.h in ESP-IDF 5.2
typedef enum {
    WIFI_AUTH_OPEN = 0,         /**< authenticate mode : open */
    WIFI_AUTH_WEP,              /**< authenticate mode : WEP */
    WIFI_AUTH_WPA_PSK,          /**< authenticate mode : WPA_PSK */
    WIFI_AUTH_WPA2_PSK,         /**< authenticate mode : WPA2_PSK */
    WIFI_AUTH_WPA_WPA2_PSK,     /**< authenticate mode : WPA_WPA2_PSK */
    WIFI_AUTH_ENTERPRISE,       /**< authenticate mode : WiFi EAP security */
    WIFI_AUTH_WPA2_ENTERPRISE = WIFI_AUTH_ENTERPRISE,  /**< authenticate mode : WiFi EAP security */
    WIFI_AUTH_WPA3_PSK,         /**< authenticate mode : WPA3_PSK */
    WIFI_AUTH_WPA2_WPA3_PSK,    /**< authenticate mode : WPA2_WPA3_PSK */
    WIFI_AUTH_WAPI_PSK,         /**< authenticate mode : WAPI_PSK */
    WIFI_AUTH_OWE,              /**< authenticate mode : OWE */
    WIFI_AUTH_WPA3_ENT_192,     /**< authenticate mode : WPA3_ENT_SUITE_B_192_BIT */
    WIFI_AUTH_WPA3_EXT_PSK,     /**< authenticate mode : WPA3_PSK_EXT_KEY */
    WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE, /**< authenticate mode: WPA3_PSK + WPA3_PSK_EXT_KEY */
    WIFI_AUTH_MAX
} wifi_auth_mode_t;
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

/**
 * @return true if wifi is actively scanning
 */
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

/**
 * Return true if the connection isn't unencrypted.
 */
bool wifi_is_connection_secure();

/**
 * Returns the RSSI value (negative number) or return 1 when not connected
 */
int wifi_get_rssi();

#ifdef __cplusplus
}
#endif
