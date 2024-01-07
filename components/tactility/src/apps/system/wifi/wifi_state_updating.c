#include "wifi_state_updating.h"
#include "esp_lvgl_port.h"

void wifi_state_set_scanning(Wifi* wifi, bool is_scanning) {
    wifi_lock(wifi);
    wifi->state.scanning = is_scanning;

    lvgl_port_lock(100);
    wifi_view_update(&wifi->view, &wifi->state);
    lvgl_port_unlock();

    wifi_unlock(wifi);
}

void wifi_state_set_radio_state(Wifi* wifi, WifiRadioState state) {
    wifi_lock(wifi);
    wifi->state.radio_state = state;

    lvgl_port_lock(100);
    wifi_view_update(&wifi->view, &wifi->state);
    lvgl_port_unlock();

    wifi_unlock(wifi);
}
