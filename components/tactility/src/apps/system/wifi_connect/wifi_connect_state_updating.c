#include "wifi_connect_state_updating.h"

#include "esp_lvgl_port.h"

void wifi_connect_state_set_radio_state(WifiConnect* wifi, WifiRadioState state) {
    wifi_connect_lock(wifi);
    wifi->state.radio_state = state;

    lvgl_port_lock(100);
    wifi_connect_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    lvgl_port_unlock();

    wifi_connect_unlock(wifi);
}
