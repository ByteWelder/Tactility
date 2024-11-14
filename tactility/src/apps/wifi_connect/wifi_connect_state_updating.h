#pragma once

#include "wifi_connect.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_connect_state_set_radio_error(WifiConnect* wifi, bool error);
void wifi_connect_state_set_ap_settings(WifiConnect* wifi, const WifiApSettings* settings);
void wifi_connect_state_set_connecting(WifiConnect* wifi, bool is_connecting);

#ifdef __cplusplus
}
#endif
