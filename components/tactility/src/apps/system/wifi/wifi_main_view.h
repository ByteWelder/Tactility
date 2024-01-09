#pragma once

#include "lvgl.h"
#include "wifi_state.h"
#include "wifi_view.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi_main_view_create(WifiView* wifi_view, lv_obj_t* parent);
void wifi_main_view_update(WifiView* view, WifiState* state);

#ifdef __cplusplus
}
#endif
