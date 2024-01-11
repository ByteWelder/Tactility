#include "wifi_manage.h"
#include "esp_lvgl_port.h"

void wifi_manage_state_set_scanning(WifiManage* wifi, bool is_scanning) {
    wifi_manage_lock(wifi);
    wifi->state.scanning = is_scanning;

    lvgl_port_lock(100);
    wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    lvgl_port_unlock();

    wifi_manage_unlock(wifi);
}

void wifi_manage_state_set_radio_state(WifiManage* wifi, WifiRadioState state) {
    wifi_manage_lock(wifi);
    wifi->state.radio_state = state;

    lvgl_port_lock(100);
    wifi_manage_view_update(&wifi->view, &wifi->bindings, &wifi->state);
    lvgl_port_unlock();

    wifi_manage_unlock(wifi);
}
