#include "wifi_connect_state_updating.h"

#include "esp_lvgl_port.h"

void wifi_connect_state_set_radio_state(WifiConnect* wifi, WifiRadioState state) {
    wifi_connect_lock(wifi);
    wifi->state.radio_state = state;
    wifi_connect_unlock(wifi);

    wifi_connect_request_view_update(wifi);
}
