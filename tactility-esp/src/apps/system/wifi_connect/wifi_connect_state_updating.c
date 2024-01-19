#include "wifi_connect_state_updating.h"

void wifi_connect_state_set_radio_state(WifiConnect* wifi, WifiRadioState state) {
    wifi_connect_lock(wifi);
    wifi->state.radio_state = state;
    wifi_connect_unlock(wifi);
}
