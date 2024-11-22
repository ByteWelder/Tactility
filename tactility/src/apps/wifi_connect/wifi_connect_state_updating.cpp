#include "wifi_connect_state_updating.h"

namespace tt::app::wifi_connect {

void state_set_radio_error(WifiConnect* wifi, bool error) {
    lock(wifi);
    wifi->state.connection_error = error;
    unlock(wifi);
}

void state_set_ap_settings(WifiConnect* wifi, const service::wifi::settings::WifiApSettings* settings) {
    lock(wifi);
    memcpy(&(wifi->state.settings), settings, sizeof(service::wifi::settings::WifiApSettings));
    unlock(wifi);
}

void state_set_connecting(WifiConnect* wifi, bool is_connecting) {
    lock(wifi);
    wifi->state.is_connecting = is_connecting;
    unlock(wifi);
}

} // namespace
