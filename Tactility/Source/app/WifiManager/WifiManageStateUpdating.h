#pragma once

#include "WifiManage.h"

namespace tt::app::wifi_manage {

void state_set_scanning(WifiManage* wifi, bool is_scanning);
void state_set_radio_state(WifiManage* wifi, service::wifi::WifiRadioState state);
void state_update_scanned_records(WifiManage* wifi);

} // namespace
