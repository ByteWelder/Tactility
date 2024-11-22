#pragma once

#include "WifiConnect.h"

namespace tt::app::wifi_connect {

void state_set_radio_error(WifiConnect* wifi, bool error);
void state_set_ap_settings(WifiConnect* wifi, const service::wifi::settings::WifiApSettings* settings);
void state_set_connecting(WifiConnect* wifi, bool is_connecting);

} // namespace
