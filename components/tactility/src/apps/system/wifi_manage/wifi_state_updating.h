#pragma once

#include "wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_state_set_scanning(Wifi* wifi, bool is_scanning);
void wifi_state_set_radio_state(Wifi* wifi, WifiRadioState state);

#ifdef __cplusplus
}
#endif
