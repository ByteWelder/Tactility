#include "wifi_connect_state_updating.h"

void wifi_connect_state_set_radio_error(WifiConnect* wifi, bool error) {
    wifi_connect_lock(wifi);
    wifi->state.connection_error = error;
    wifi_connect_unlock(wifi);
}

void wifi_connect_state_set_ap_settings(WifiConnect* wifi, const WifiApSettings* settings) {
    wifi_connect_lock(wifi);
    memcpy(&(wifi->state.settings), settings, sizeof(WifiApSettings));
    wifi_connect_unlock(wifi);
}
