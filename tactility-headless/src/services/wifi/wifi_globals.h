#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define TT_WIFI_AUTO_CONNECT true // Default setting for new Wi-Fi entries
#define TT_WIFI_AUTO_ENABLE true

#define TT_WIFI_SCAN_RECORD_LIMIT 16 // default, can be overridden

#define TT_WIFI_SSID_LIMIT 32 // 32 characters/octets, according to IEEE 802.11-2020 spec
#define TT_WIFI_CREDENTIALS_PASSWORD_LIMIT 64 // 64 characters/octets, according to IEEE 802.11-2020 spec

typedef struct {
    char ssid[TT_WIFI_SSID_LIMIT + 1]; // Add extra character for null termination
    char secret[TT_WIFI_CREDENTIALS_PASSWORD_LIMIT + 1]; // Add extra character for null termination
    bool auto_connect;
} WifiApSettings;

#ifdef __cplusplus
}
#endif
