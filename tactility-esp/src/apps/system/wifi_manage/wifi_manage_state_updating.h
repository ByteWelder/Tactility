#pragma once

#include "wifi_manage.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_manage_state_set_scanning(WifiManage* wifi, bool is_scanning);
void wifi_manage_state_set_radio_state(WifiManage* wifi, WifiRadioState state);
void wifi_manage_state_update_scanned_records(WifiManage* wifi);

#ifdef __cplusplus
}
#endif
