#pragma once

#include <stdbool.h>
#include <stdint.h>

#define TT_WIFI_SSID_LIMIT 32 // 32 characters/octets, according to IEEE 802.11-2020 spec
#define TT_WIFI_CREDENTIALS_PASSWORD_LIMIT 64 // 64 characters/octets, according to IEEE 802.11-2020 spec

#ifdef __cplusplus
extern "C" {
#endif

/** Important: These values must map to tt::service::wifi::RadioState values exactly */
typedef enum {
    WIFI_RADIO_STATE_ON_PENDING,
    WIFI_RADIO_STATE_ON,
    WIFI_RADIO_STATE_CONNECTION_PENDING,
    WIFI_RADIO_STATE_CONNECTION_ACTIVE,
    WIFI_RADIO_STATE_OFF_PENDING,
    WIFI_RADIO_STATE_OFF,
} WifiRadioState;

/** @return the state of the WiFi radio */
WifiRadioState tt_wifi_get_radio_state();

/** @return a textual representation of the WiFi radio state */
const char* tt_wifi_radio_state_to_string(WifiRadioState state);

/** Start scanning */
void tt_wifi_scan();

/** @return true if a scan is active/pending */
bool tt_wifi_is_scanning();

/**
 * Return the WiFi SSID that the system tries to connect to, or is connected to.
 * @param[out] buffer an allocated string buffer. Its size must be (WIFI_SSID_LIMIT + 1).
 */
void tt_wifi_get_connection_target(char* buffer);

/**
 * @brief Enable/disable the radio. Ignores input if desired state matches current state.
 * @param[in] enabled
 */
void tt_wifi_set_enabled(bool enabled);

/**
 *
 * @param ssid The access point identifier - maximal 32 characters/octets
 * @param password the password - maximum 64 characters/octets
 * @param channel 0 means "any"
 * @param autoConnect whether we want to automatically reconnect if a disconnect occurs
 * @param remember whether the record should be stored permanently on the device (it is only stored if this connection attempt succeeds)
 */
void tt_wifi_connect(const char* ssid, const char* password, int32_t channel, bool autoConnect, bool remember);

/**
 * If WiFi is connected, this disconnects it.
 */
void tt_wifi_disconnect();

/**
 * @return true if WiFi is active and encrypted
 */
bool tt_wifi_is_connnection_secure();

/**
 * @return the current radio connection link quality
 */
int tt_wifi_get_rssi();

#ifdef __cplusplus
}
#endif
