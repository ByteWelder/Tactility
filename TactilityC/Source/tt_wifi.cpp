#include "tt_wifi.h"

#include <cstring>
#include <Tactility/service/wifi/Wifi.h>
#include <Tactility/service/wifi/WifiSettings.h>

using namespace tt::service;

extern "C" {

WifiRadioState tt_wifi_get_radio_state() {
    return static_cast<WifiRadioState>(wifi::getRadioState());
}
const char* tt_wifi_radio_state_to_string(WifiRadioState state) {
    return wifi::radioStateToString(static_cast<wifi::RadioState>(state));
}

void tt_wifi_scan() {
    wifi::scan();
}

bool tt_wifi_is_scanning() {
    return wifi::isScanning();
}

void tt_wifi_get_connection_target(char* buffer) {
    auto target = wifi::getConnectionTarget();
    strcpy(buffer, target.c_str());
}

void tt_wifi_set_enabled(bool enabled) {
    wifi::setEnabled(enabled);
}

void tt_wifi_connect(const char* ssid, const char* password, int32_t channel, bool autoConnect, bool remember) {
    wifi::settings::WifiApSettings settings;
    settings.ssid = ssid;
    settings.password = password;
    settings.channel = channel;
    settings.autoConnect = autoConnect;
    wifi::connect(settings, remember);
}

void tt_wifi_disconnect() {
    wifi::disconnect();
}

bool tt_wifi_is_connnection_secure() {
    return wifi::isConnectionSecure();
}

int tt_wifi_get_rssi() {
    return wifi::getRssi();
}

}
