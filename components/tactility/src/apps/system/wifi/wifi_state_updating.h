#pragma once

#include "wifi.h"

void wifi_state_set_scanning(Wifi* wifi, bool is_scanning);
void wifi_state_set_radio_state(Wifi* wifi, WifiRadioState state);
