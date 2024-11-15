#include "wifi_manage.h"

void wifi_manage_state_set_scanning(WifiManage* wifi, bool is_scanning) {
    wifi_manage_lock(wifi);
    wifi->state.scanning = is_scanning;
    wifi_manage_unlock(wifi);
}

void wifi_manage_state_set_radio_state(WifiManage* wifi, WifiRadioState state) {
    wifi_manage_lock(wifi);
    wifi->state.radio_state = state;
    wifi_manage_unlock(wifi);
}

void wifi_manage_state_update_scanned_records(WifiManage* wifi) {
    wifi_manage_lock(wifi);
    wifi_get_scan_results(
        wifi->state.ap_records,
        WIFI_SCAN_AP_RECORD_COUNT,
        &wifi->state.ap_records_count
    );
    wifi_manage_unlock(wifi);
}