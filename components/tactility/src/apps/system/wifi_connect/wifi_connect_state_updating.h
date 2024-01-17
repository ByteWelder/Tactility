#pragma once

#include "wifi_connect.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_connect_state_set_radio_state(WifiConnect* wifi, WifiRadioState state);

#ifdef __cplusplus
}
#endif
