#include "WifiManage.h"

namespace tt::app::wifi_manage {

void state_set_scanning(WifiManage* wifi, bool is_scanning) {
    lock(wifi);
    wifi->state.scanning = is_scanning;
    unlock(wifi);
}

void state_set_radio_state(WifiManage* wifi, service::wifi::WifiRadioState state) {
    lock(wifi);
    wifi->state.radio_state = state;
    unlock(wifi);
}

void state_update_scanned_records(WifiManage* wifi) {
    lock(wifi);
    service::wifi::get_scan_results(
        wifi->state.ap_records,
        WIFI_SCAN_AP_RECORD_COUNT,
        &wifi->state.ap_records_count
    );
    unlock(wifi);
}

} // namespace
